package db

import (
	"fmt"
	"time"

	"github.com/z3vxo/Abyss/server/internal/shared"
)

func (db *DB) InsertAgent(info shared.ClientRegister) error {
	q := `INSERT INTO agents
		(AgentName, AgentId, LastSeen, CreatedAt, IsAdmin, PID, TID, PPID, Arch, Username, Hostname, ProcPath, Domain, WinVer, IpAddr)
		VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)`
	now := time.Now().Unix()
	_, err := db.conn.Exec(q,
		fmt.Sprintf("agent-%d", info.AgentId),
		info.AgentId, now, now, info.IsAdmin,
		info.Pid, info.Tid, info.PPid, info.Arch,
		info.Username, info.Hostname, info.ProcPath, info.Domain,
		info.WinVer, "")
	return err
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
	Arch      int    `json:"arch"`
	IsAdmin   int    `json:"is_admin"`
	LastSeen  int    `json:"last_seen"`
	CreatedAt int    `json:"created_at"`
}

// `CREATE TABLE IF NOT EXISTS agents (
// 		id        INTEGER PRIMARY KEY AUTOINCREMENT,
// 		AgentName TEXT    NOT NULL UNIQUE,
// 		AgentId   INTEGER NOT NULL UNIQUE,
// 		LastSeen  INTEGER NOT NULL,
// 		CreatedAt INTEGER NOT NULL,
// 		IsAdmin   INTEGER NOT NULL,
// 		PID       INTEGER NOT NULL,
// 		TID       INTEGER NOT NULL,
// 		PPID      INTEGER NOT NULL,
// 		Arch      INTEGER NOT NULL,
// 		Username  TEXT    NOT NULL,
// 		Hostname  TEXT    NOT NULL,
// 		ProcPath  TEXT    NOT NULL,
// 		Domain    TEXT    NOT NULL,
// 		WinVer    TEXT    NOT NULL,
// 		IpAddr    TEXT    NOT NULL
// 	);`,

func (db *DB) ListAgents() (*AgentsResp, error) {
	rows, err := db.conn.Query(`SELECT AgentName, AgentId, Username, Hostname, Domain, ProcPath, WinVer, PID, TID, PPID, Arch, IsAdmin, LastSeen, CreatedAt FROM agents ORDER BY CreatedAt;`)
	if err != nil {
		return &AgentsResp{}, err
	}
	defer rows.Close()

	agents := make([]Agent, 0)
	for rows.Next() {
		var a Agent
		if err := rows.Scan(&a.AgentName, &a.AgentId, &a.Username, &a.Hostname, &a.Domain, &a.ProcPath, &a.WinVer, &a.PID, &a.TID, &a.PPID, &a.Arch, &a.IsAdmin, &a.LastSeen, &a.CreatedAt); err != nil {
			return &AgentsResp{}, err
		}
		agents = append(agents, a)
	}
	if err := rows.Err(); err != nil {
		return &AgentsResp{}, err
	}

	return &AgentsResp{
		Total:  len(agents),
		Agents: agents,
	}, nil
}
