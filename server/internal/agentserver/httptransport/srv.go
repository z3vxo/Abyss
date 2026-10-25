package httptransport

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io"
	"net/http"

	"github.com/z3vxo/Abyss/server/internal/db"
)

type AgentServerInfo struct {
	DB   *db.DB
	Host string
}

const (
	CMD_TYPE_REGISTER = 1
)

func (srv *AgentServerInfo) HandlePost(w http.ResponseWriter, r *http.Request) {
	host := r.Header.Get("Host")
	if host != srv.Host {
		w.WriteHeader(http.StatusNotAcceptable)
		return
	}

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

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"Status": "listener working!"})
}
