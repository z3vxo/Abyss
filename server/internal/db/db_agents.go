package db

import (
	"fmt"
	"math/rand"
	"time"

	"github.com/z3vxo/Abyss/server/internal/shared"
)

var (
	agentAdjectives = []string{
		"ancient", "ashen", "bitter", "broken", "crimson", "crawling",
		"distant", "drowned", "feral", "forgotten", "frozen", "grim",
		"hollow", "lurking", "pale", "quiet", "restless", "shadowed",
		"silent", "sleepless", "unseen", "veiled", "wandering", "withered",
	}
	agentNouns = []string{
		"abyss", "crow", "echo", "ghoul", "herald", "jackal",
		"kraken", "leviathan", "moth", "oracle", "phantom", "raven",
		"revenant", "sentinel", "serpent", "shade", "specter", "tide",
		"veil", "viper", "wanderer", "wolf", "wraith", "hollow",
	}
)

func randomAgentName() string {
	adjective := agentAdjectives[rand.Intn(len(agentAdjectives))]
	noun := agentNouns[rand.Intn(len(agentNouns))]
	return fmt.Sprintf("%s-%s", adjective, noun)
}

func (db *DB) InsertAgent(info *shared.ClientRegister) error {
	const q = `INSERT INTO agents
		(AgentName, AgentId, LastSeen, CreatedAt, IsAdmin, PID, TID, PPID, Arch, Username, Hostname, ProcPath, Domain, WinVer, IpAddr)
		VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
		ON CONFLICT(AgentName) DO NOTHING`

	now := time.Now().Unix()
	for attempt := 0; attempt < 100; attempt++ {
		name := randomAgentName()
		res, err := db.conn.Exec(q, name,
			info.AgentId, now, now, info.IsAdmin,
			info.Pid, info.Tid, info.PPid, info.Arch,
			info.Username, info.Hostname, info.ProcPath, info.Domain,
			info.WinVer, "")
		if err != nil {
			return err
		}

		n, err := res.RowsAffected()
		if err != nil {
			return err
		}
		if n == 0 {
			continue
		}

		info.AgentName = name
		return nil
	}

	return fmt.Errorf("could not generate a unique agent name")
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
	Arch      string `json:"arch"`
	IsAdmin   string `json:"is_admin"`
	LastSeen  int    `json:"last_seen"`
	CreatedAt int    `json:"created_at"`
}

func archLabel(arch int) string {
	if arch == 1 {
		return "x64"
	}
	return "x86"
}

func adminLabel(isAdmin int) string {
	if isAdmin == 1 {
		return "TRUE"
	}
	return "FALSE"
}

func (db *DB) UpdateAgentSeen(id int32) error {

	_, err := db.conn.Exec(`UPDATE agents SET LastSeen = ? WHERE AgentID = ?`, time.Now().Unix(), id)
	return err
}
func (db *DB) ListAgents() (*AgentsResp, error) {
	rows, err := db.conn.Query(`SELECT AgentName, AgentId, Username, Hostname, Domain, ProcPath, WinVer, PID, TID, PPID, Arch, IsAdmin, LastSeen, CreatedAt FROM agents ORDER BY CreatedAt;`)
	if err != nil {
		return &AgentsResp{}, err
	}
	defer rows.Close()

	agents := make([]Agent, 0)
	for rows.Next() {
		var a Agent
		var arch, isAdmin int
		if err := rows.Scan(&a.AgentName, &a.AgentId, &a.Username, &a.Hostname, &a.Domain, &a.ProcPath, &a.WinVer, &a.PID, &a.TID, &a.PPID, &arch, &isAdmin, &a.LastSeen, &a.CreatedAt); err != nil {
			return &AgentsResp{}, err
		}
		a.Arch = archLabel(arch)
		a.IsAdmin = adminLabel(isAdmin)
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
