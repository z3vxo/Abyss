package httptransport

import (
	"encoding/json"
	"net/http"

	"github.com/z3vxo/Abyss/internal/db"
)

type AgentServerInfo struct {
	DB   *db.DB
	Host string
}

func (srv *AgentServerInfo) HandlePost(w http.ResponseWriter, r *http.Request) {
	host := r.Header.Get("Host")
	if host != srv.Host {
		w.WriteHeader(http.StatusNotAcceptable)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"Status": "listener working!"})
}
