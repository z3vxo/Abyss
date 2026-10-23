package listeners

import (
	"context"
	"crypto/tls"
	"errors"
	"fmt"
	"net"
	"net/http"
	"sort"
	"sync"

	"github.com/google/uuid"
	"github.com/z3vxo/Abyss/internal/shared"
)

type Status string

const (
	StatusPaused   Status = "paused"
	StatusStarting Status = "starting"
	StatusRunning  Status = "running"
	StatusStopping Status = "stopping"
	StatusStopped  Status = "stopped"
	StatusError    Status = "error"
)

type ListenerType string

const (
	TypeHTTP  ListenerType = "http"
	TypeHTTPS ListenerType = "https"
)

type AgentHandler interface {
	HandlePost(http.ResponseWriter, *http.Request)
}

type Listener struct {
	ID     string
	Name   string
	Type   ListenerType
	Bind   string
	Port   int
	Status Status

	Info   shared.HttpListenerInfo
	Server *http.Server

	tls *tls.Config
}

type ListenerInfo struct {
	ID     string `json:"id"`
	Name   string `json:"name"`
	Type   string `json:"type"`
	Bind   string `json:"bind_addr"`
	Port   int    `json:"port"`
	Status string `json:"status"`
}

type Listeners struct {
	mu          sync.RWMutex
	ListenerMap map[string]*Listener
	agent       AgentHandler
}

func Create(agent AgentHandler) *Listeners {
	return &Listeners{
		ListenerMap: make(map[string]*Listener),
		agent:       agent,
	}
}

func (l *Listeners) NewHttpListener(info shared.HttpListenerInfo, typ ListenerType, name string) (string, error) {
	return l.newHttpListener("", info, typ, name)
}

func (l *Listeners) RestoreHttpListener(id string, info shared.HttpListenerInfo, typ ListenerType, name string) error {
	_, err := l.newHttpListener(id, info, typ, name)
	return err
}

func (l *Listeners) newHttpListener(id string, info shared.HttpListenerInfo, typ ListenerType, name string) (string, error) {
	if name == "" {
		return "", errors.New("listener name is required")
	}

	tlsCfg, err := BuildTLS(info, typ)
	if err != nil {
		return "", err
	}

	srv, err := BuildServer(info, tlsCfg, l.agent)
	if err != nil {
		return "", err
	}

	if id == "" {
		generated, err := uuid.NewV7()
		if err != nil {
			return "", fmt.Errorf("generate listener id: %w", err)
		}
		id = generated.String()
	}

	l.mu.Lock()
	defer l.mu.Unlock()

	for _, existing := range l.ListenerMap {
		if existing.Bind == info.Bind && existing.Port == info.Port {
			return "", fmt.Errorf("listener already bound to %s:%d", info.Bind, info.Port)
		}
	}

	l.ListenerMap[id] = &Listener{
		ID:     id,
		Name:   name,
		Type:   typ,
		Bind:   info.Bind,
		Port:   info.Port,
		Status: StatusPaused,
		Info:   info,
		Server: srv,
		tls:    tlsCfg,
	}

	return id, nil
}

func (l *Listeners) New(info shared.NewListenerReq) (string, error) {
	switch ListenerType(info.Type) {
	case TypeHTTP, TypeHTTPS:
		return l.NewHttpListener(info.HttpInfo, ListenerType(info.Type), info.Name)
	default:
		return "", fmt.Errorf("unsupported listener type %q", info.Type)
	}
}

func (l *Listeners) Restore(info shared.NewListenerReq, id string) error {
	switch ListenerType(info.Type) {
	case TypeHTTP, TypeHTTPS:
		return l.RestoreHttpListener(id, info.HttpInfo, ListenerType(info.Type), info.Name)
	default:
		return fmt.Errorf("unsupported listener type %q", info.Type)
	}
}

func (l *Listeners) Remove(id string) error {
	l.mu.Lock()
	defer l.mu.Unlock()

	listener, ok := l.ListenerMap[id]
	if !ok {
		return fmt.Errorf("listener %q not found", id)
	}
	if listener.Status == StatusRunning || listener.Status == StatusStarting || listener.Status == StatusStopping {
		return fmt.Errorf("cannot remove listener %q while %s", id, listener.Status)
	}

	delete(l.ListenerMap, id)
	return nil
}

func (l *Listeners) Start(id string) error {
	l.mu.Lock()
	listener, ok := l.ListenerMap[id]
	if !ok {
		l.mu.Unlock()
		return fmt.Errorf("listener %q not found", id)
	}
	if listener.Status == StatusRunning || listener.Status == StatusStarting {
		l.mu.Unlock()
		return fmt.Errorf("listener %q is already running", id)
	}

	listener.Status = StatusStarting
	srv := listener.Server
	info := listener.Info
	tlsCfg := listener.tls
	typ := listener.Type
	l.mu.Unlock()

	if srv == nil {
		built, err := BuildServer(info, tlsCfg, l.agent)
		if err != nil {
			l.setStatus(listener, StatusError)
			return err
		}
		srv = built
	}

	ln, err := net.Listen("tcp", srv.Addr)
	if err != nil {
		l.setStatus(listener, StatusError)
		return fmt.Errorf("listen on %s: %w", srv.Addr, err)
	}

	l.mu.Lock()
	listener.Server = srv
	listener.Status = StatusRunning
	l.mu.Unlock()

	go func() {
		var serveErr error
		if typ == TypeHTTPS {
			serveErr = srv.ServeTLS(ln, "", "")
		} else {
			serveErr = srv.Serve(ln)
		}

		if serveErr != nil && !errors.Is(serveErr, http.ErrServerClosed) {
			l.mu.Lock()
			listener.Status = StatusError
			listener.Server = nil
			l.mu.Unlock()
		}
	}()

	return nil
}

func (l *Listeners) Stop(id string, ctx context.Context) error {
	if ctx == nil {
		ctx = context.Background()
	}

	l.mu.Lock()
	listener, ok := l.ListenerMap[id]
	if !ok {
		l.mu.Unlock()
		return fmt.Errorf("listener %q not found", id)
	}
	if listener.Status != StatusRunning && listener.Status != StatusStarting {
		l.mu.Unlock()
		return fmt.Errorf("listener %q is not running", id)
	}

	listener.Status = StatusStopping
	srv := listener.Server
	l.mu.Unlock()

	var shutdownErr error
	if srv != nil {
		shutdownErr = srv.Shutdown(ctx)
	}

	l.mu.Lock()
	listener.Server = nil
	if shutdownErr != nil {
		listener.Status = StatusError
	} else {
		listener.Status = StatusStopped
	}
	l.mu.Unlock()

	if shutdownErr != nil {
		return fmt.Errorf("stop listener %q: %w", id, shutdownErr)
	}
	return nil
}

func (l *Listeners) List() []ListenerInfo {
	l.mu.RLock()
	out := make([]ListenerInfo, 0, len(l.ListenerMap))
	for _, listener := range l.ListenerMap {
		out = append(out, ListenerInfo{
			ID:     listener.ID,
			Name:   listener.Name,
			Type:   string(listener.Type),
			Bind:   listener.Bind,
			Port:   listener.Port,
			Status: string(listener.Status),
		})
	}
	l.mu.RUnlock()

	sort.Slice(out, func(i, j int) bool { return out[i].ID < out[j].ID })
	return out
}

func (l *Listeners) setStatus(listener *Listener, status Status) {
	l.mu.Lock()
	listener.Status = status
	l.mu.Unlock()
}
