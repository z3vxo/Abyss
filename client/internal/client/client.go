package client

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/url"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/z3vxo/Abyss/client/internal/config"
	"github.com/z3vxo/Abyss/client/internal/ws"
)

type Client struct {
	base *url.URL
	http *http.Client

	Events chan ws.Event

	mu      sync.RWMutex
	access  string
	refresh string
}

type tokens struct {
	Access  string `json:"access"`
	Refresh string `json:"refresh"`
}

type loginReq struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

type APIError struct {
	Status  int
	Message string
}

func (e *APIError) Error() string {
	return fmt.Sprintf("%s (HTTP %d)", e.Message, e.Status)
}

const (
	loginRetries = 3
	retryDelay   = 2 * time.Second
)

func New(cfg *config.Config) (*Client, error) {
	base, err := buildBaseURL(cfg.ConnectionCfg.Host, cfg.ConnectionCfg.Port)
	if err != nil {
		return nil, err
	}

	c := &Client{
		base:   base,
		http:   &http.Client{Timeout: 15 * time.Second},
		Events: make(chan ws.Event, 128),
	}

	for attempt := 0; attempt <= loginRetries; attempt++ {
		if attempt > 0 {
			fmt.Println("[!] Failed connecting to teamserver, retrying...")
			time.Sleep(retryDelay)
		}

		err := c.Login(context.Background(), cfg.ConnectionCfg.Username, cfg.ConnectionCfg.Password)
		if err == nil {
			err = c.ConnectWS(context.Background())
			if err == nil {
				fmt.Println("[*] Connected to teamserver")
				return c, nil
			}
		}

		var apiErr *APIError
		if errors.As(err, &apiErr) && apiErr.Status < 500 {
			return nil, fmt.Errorf("connect to teamserver: %w", err)
		}

		if attempt == loginRetries {
			return nil, fmt.Errorf("connect to teamserver: %w", err)
		}
	}

	return nil, fmt.Errorf("connect to teamserver: unreachable")
}

type NewListenerResp struct {
	ListenerId string `json:"id"`
}

type ListenerInfo struct {
	ID     string `json:"id"`
	Name   string `json:"name"`
	Type   string `json:"type"`
	Bind   string `json:"bind_addr"`
	Port   int    `json:"port"`
	Status string `json:"status"`
}

func (c *Client) ListListeners(ctx context.Context) ([]ListenerInfo, error) {
	var out []ListenerInfo
	if err := c.do(ctx, http.MethodGet, "/rest/listener/list", nil, &out); err != nil {
		return nil, fmt.Errorf("list listeners: %w", err)
	}

	return out, nil
}

func (c *Client) StartListener(ctx context.Context, id string) error {
	if err := c.do(ctx, http.MethodPost, "/rest/listener/start/"+id, nil, nil); err != nil {
		return fmt.Errorf("start listener: %w", err)
	}
	return nil
}

func (c *Client) StopListener(ctx context.Context, id string) error {
	if err := c.do(ctx, http.MethodPatch, "/rest/listener/stop/"+id, nil, nil); err != nil {
		return fmt.Errorf("stop listener: %w", err)
	}
	return nil
}

func (c *Client) DeleteListener(ctx context.Context, id string) error {
	if err := c.do(ctx, http.MethodDelete, "/rest/listener/delete/"+id, nil, nil); err != nil {
		return fmt.Errorf("delete listener: %w", err)
	}
	return nil
}

func (c *Client) NewListener(ctx context.Context, req any) (string, error) {
	var resp NewListenerResp
	if err := c.do(ctx, http.MethodPost, "/rest/listener/new", req, &resp); err != nil {
		return "", fmt.Errorf("new listener: %w", err)
	}

	return resp.ListenerId, nil
}

type NewTaskResp struct {
	TaskID int32 `json:"task_id"`
}

func (c *Client) NewTask(ctx context.Context, data any) (int32, error) {
	var resp NewTaskResp
	if err := c.do(ctx, http.MethodPost, "/rest/tasks/new", data, &resp); err != nil {
		return 0, fmt.Errorf("new-task: %w", err)
	}
	return resp.TaskID, nil
}

func (c *Client) Login(ctx context.Context, user, pass string) error {
	var out tokens
	if err := c.do(ctx, http.MethodPost, "/rest/auth/login", loginReq{Username: user, Password: pass}, &out); err != nil {
		return fmt.Errorf("login: %w", err)
	}

	if out.Access == "" || out.Refresh == "" {
		return fmt.Errorf("login: response is missing tokens")
	}

	c.mu.Lock()
	c.access = out.Access
	c.refresh = out.Refresh
	c.mu.Unlock()

	return nil
}

func (c *Client) ConnectWS(ctx context.Context) error {
	c.mu.RLock()
	token := c.access
	c.mu.RUnlock()
	if token == "" {
		return fmt.Errorf("connect websocket: no access token")
	}

	u := *c.base
	switch u.Scheme {
	case "http":
		u.Scheme = "ws"
	case "https":
		u.Scheme = "wss"
	default:
		return fmt.Errorf("connect websocket: unsupported scheme %q", u.Scheme)
	}
	u.Path = strings.TrimSuffix(u.Path, "/") + "/rest/ws/connect"
	q := u.Query()
	q.Set("token", token)
	u.RawQuery = q.Encode()

	dialCtx, cancel := context.WithTimeout(ctx, 15*time.Second)
	defer cancel()

	conn, err := ws.Dial(dialCtx, u.String())
	if err != nil {
		return fmt.Errorf("connect websocket: %w", err)
	}

	go c.readWS(conn)
	return nil
}

func (c *Client) readWS(conn *ws.Conn) {
	defer close(c.Events)
	_ = conn.ReadLoop(context.Background(), func(event ws.Event) {
		c.Events <- event
	})
}

func (c *Client) do(ctx context.Context, method, path string, in, out any) error {
	var body io.Reader
	if in != nil {
		data, err := json.Marshal(in)
		if err != nil {
			return fmt.Errorf("encode request: %w", err)
		}
		body = bytes.NewReader(data)
	}

	req, err := http.NewRequestWithContext(ctx, method, c.base.JoinPath(path).String(), body)
	if err != nil {
		return fmt.Errorf("build request: %w", err)
	}
	req.Header.Set("Accept", "application/json")
	if in != nil {
		req.Header.Set("Content-Type", "application/json")
	}

	c.mu.RLock()
	if c.access != "" {
		req.Header.Set("Authorization", "Bearer "+c.access)
	}
	c.mu.RUnlock()

	resp, err := c.http.Do(req)
	if err != nil {
		return fmt.Errorf("request: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		var apiErr struct {
			Error string `json:"error"`
		}
		_ = json.NewDecoder(io.LimitReader(resp.Body, 1<<20)).Decode(&apiErr)
		if apiErr.Error == "" {
			apiErr.Error = http.StatusText(resp.StatusCode)
		}
		return &APIError{Status: resp.StatusCode, Message: apiErr.Error}
	}

	if out != nil {
		if err := json.NewDecoder(io.LimitReader(resp.Body, 1<<20)).Decode(out); err != nil {
			return fmt.Errorf("decode response: %w", err)
		}
	}

	return nil
}

func buildBaseURL(host string, port int) (*url.URL, error) {
	if host == "" {
		return nil, fmt.Errorf("connection host is required")
	}
	if !strings.Contains(host, "://") {
		host = "http://" + host
	}

	u, err := url.Parse(host)
	if err != nil {
		return nil, fmt.Errorf("parse host %q: %w", host, err)
	}
	if u.Hostname() == "" {
		return nil, fmt.Errorf("invalid host %q", host)
	}

	if u.Port() == "" && port != 0 {
		u.Host = net.JoinHostPort(u.Hostname(), strconv.Itoa(port))
	}

	return u, nil
}

type AgentsResp struct {
	Total  int     `json:"total"`
	Agents []Agent `json:"agent"`
}

type Agent struct {
	AgentId   int32  `json:"agent_id"`
	AgentName string `json:"agent_name"`
	Username  string `json:"username"`
	Hostname  string `json:"hostname"`
	Domain    string `json:"domain"`
	ProcPath  string `json:"proc_path"`
	WinVer    string `json:"winver"`
	PID       int    `json:"pid"`
	TID       int    `json:"tid"`
	PPID      int    `json:"ppid"`
	Arch      string `json:"arch"`
	IsAdmin   string `json:"is_admin"`
	LastSeen  int    `json:"last_seen"`
	CreatedAt int    `json:"created_at"`
}

func (c *Client) ListAgents(ctx context.Context) (AgentsResp, error) {
	var out AgentsResp
	if err := c.do(ctx, http.MethodGet, "/rest/agents/list", nil, &out); err != nil {
		return AgentsResp{}, fmt.Errorf("list agents: %w", err)
	}

	return out, nil
}
