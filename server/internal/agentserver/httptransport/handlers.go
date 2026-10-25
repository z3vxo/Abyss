package httptransport

import (
	"bytes"

	"github.com/z3vxo/Abyss/server/internal/parser"
)

func (srv *AgentServerInfo) HandleRegister(r *bytes.Reader) error {

	clientInfo, err := parser.ParseRegister(r)
	if err != nil {
		return err
	}

	if err := srv.DB.InsertAgent(clientInfo); err != nil {
		return err
	}

	return nil
}
