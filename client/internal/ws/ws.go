package ws

import (
	"context"
	"encoding/json"
	"fmt"

	"github.com/coder/websocket"
)

const (
	EventNewAgent   = "new-agent"
	EventTaskOutput = "task-output"
)

type Event struct {
	Event string          `json:"event"`
	Body  json.RawMessage `json:"body"`
}

func (e Event) Decode(v any) error {
	return json.Unmarshal(e.Body, v)
}

type NewAgent struct {
	AgentID   int32  `json:"agent_id"`
	AgentName string `json:"agent_name"`
	Username  string `json:"username"`
	Hostname  string `json:"hostname"`
	Domain    string `json:"domain"`
	IsAdmin   string `json:"is_admin"`
}

type AgentOutput struct {
	TaskID int32  `json:"task_id"`
	Output string `json:"output"`
}

type TaskOutput struct {
	AgentID   int32         `json:"agent_id"`
	TotalSize int32         `json:"total_size"`
	Output    []AgentOutput `json:"output"`
}

type Handler func(Event)

type Conn struct {
	conn *websocket.Conn
}

func Dial(ctx context.Context, url string) (*Conn, error) {
	conn, _, err := websocket.Dial(ctx, url, nil)
	if err != nil {
		return nil, err
	}
	return &Conn{conn: conn}, nil
}

func (c *Conn) ReadLoop(ctx context.Context, handler Handler) error {
	defer c.conn.Close(websocket.StatusNormalClosure, "")

	for {
		_, data, err := c.conn.Read(ctx)
		if err != nil {
			return fmt.Errorf("websocket read: %w", err)
		}

		var event Event
		if err := json.Unmarshal(data, &event); err != nil {
			continue
		}
		if handler != nil {
			handler(event)
		}
	}
}

func (c *Conn) Close() error {
	return c.conn.Close(websocket.StatusNormalClosure, "")
}
