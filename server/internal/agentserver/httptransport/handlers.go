package httptransport

import (
	"bytes"
	"encoding/json"
	"fmt"

	"github.com/z3vxo/Abyss/server/internal/parser"
)

type NewAgentWsBody struct {
	AgentId   int32  `json:"agent_id"`
	AgentName string `json:"agent_name"`
	Username  string `json:"username"`
	Hostname  string `json:"hostname"`
	Domain    string `json:"domain"`
	IsAdmin   string `json:"is_admin"`
}

type WebsockMsg struct {
	Event string          `json:"event"`
	Body  json.RawMessage `json:"body"`
}

func (srv *AgentServerInfo) HandleRegister(r *bytes.Reader) error {

	clientInfo, err := parser.ParseRegister(r)
	if err != nil {
		fmt.Println("failed Parsing")
		return err
	}

	if err := srv.DB.InsertAgent(clientInfo); err != nil {
		fmt.Println("failed inserting to dv")

		return err
	}

	var isElev string
	if clientInfo.IsAdmin == 1 {
		isElev = "TRUE"
	} else {
		isElev = "FALSE"
	}

	body := NewAgentWsBody{
		AgentId:   clientInfo.AgentId,
		AgentName: clientInfo.AgentName,
		Username:  clientInfo.Username,
		Hostname:  clientInfo.Hostname,
		Domain:    clientInfo.Domain,
		IsAdmin:   isElev,
	}

	bodyJSON, _ := json.Marshal(body)

	msg := &WebsockMsg{
		Event: "new-agent",
		Body:  bodyJSON,
	}

	if err := srv.WS.Broadcast(msg); err != nil {
		fmt.Println(err)

		return err
	}

	return nil
}

func (srv *AgentServerInfo) HandleOutput(r *bytes.Reader) error {

	_, err := parser.ParseOutput(r)
	if err != nil {
		fmt.Println(err)

		return err
	}

	return nil
}
