package httptransport

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
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
)

func (srv *AgentServerInfo) HandlePost(w http.ResponseWriter, r *http.Request) {
	defer r.Body.Close()
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "", http.StatusNotFound)
		return
	}
	reader := bytes.NewReader(body)
	var cmdType int32
	if err := binary.Read(reader, binary.LittleEndian, &cmdType); err != nil {
		fmt.Println("Failed reading code")
		http.Error(w, "", http.StatusNotFound)
		return
	}

	switch cmdType {
	case CMD_TYPE_REGISTER:
		if err := srv.HandleRegister(reader); err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
	default:
		http.Error(w, "unknown command type", http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"Status": "listener working!"})
}
