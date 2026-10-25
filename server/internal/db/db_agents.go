package db

import (
	"time"

	"github.com/z3vxo/Abyss/server/internal/shared"
)

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

func (db *DB) InsertAgent(info shared.ClientRegister) error {
	q := `INSERT INTO agents(AgentName, AgentId, Username, Hostname, Domain, ProcPath, WinVer, IpAddr, IsAdmin, PID, TID, PPID, Arch, CreatedAt) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,);`
	_, err := db.conn.Exec(q, "test", info.AgentId, info.Username, info.Domain, info.ProcPath, "192.168.1.1", info.Pid, info.Tid, info.PPid, info.Arch, time.Now().Unix())
	return err

}
