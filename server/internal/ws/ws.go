package ws

import (
	"context"
	"encoding/json"
	"net/http"
	"sync"
	"time"

	"github.com/coder/websocket"
)

var (
	writeTimout = 10 * time.Second
)

type WS struct {
	mu      sync.RWMutex
	Clients map[*websocket.Conn]struct{}
}

func New() *WS {
	return &WS{Clients: make(map[*websocket.Conn]struct{})}
}

func (ws *WS) Add(conn *websocket.Conn) {
	ws.mu.Lock()
	ws.Clients[conn] = struct{}{}
	ws.mu.Unlock()
}

func (ws *WS) Remove(conn *websocket.Conn) {
	ws.mu.Lock()
	delete(ws.Clients, conn)
	ws.mu.Unlock()
}

func (ws *WS) Upgrade(w http.ResponseWriter, r *http.Request) error {
	conn, err := websocket.Accept(w, r, nil)
	if err != nil {
		return err
	}
	ws.Add(conn)
	defer ws.Remove(conn)
	defer conn.Close(websocket.StatusNormalClosure, "")

	for {
		if _, _, err := conn.Read(r.Context()); err != nil {
			return err
		}
	}
}

func (ws *WS) Broadcast(data any) error {
	payload, err := json.Marshal(data)
	if err != nil {
		return err
	}
	ws.mu.RLock()
	clients := make([]*websocket.Conn, 0, len(ws.Clients))
	for conn := range ws.Clients {
		clients = append(clients, conn)
	}
	ws.mu.RUnlock()
	var first error

	for _, conn := range clients {
		ctx, cancel := context.WithTimeout(context.Background(), writeTimout)
		err := conn.Write(ctx, websocket.MessageText, payload)
		cancel()

		if err != nil {
			if first == nil {
				first = err
			}
			ws.Remove(conn)
		}
	}
	return first
}
