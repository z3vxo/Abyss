package db

import (
	"database/sql"
	"fmt"
	"os"
	"path/filepath"

	_ "modernc.org/sqlite"
)

var migrations = []string{
	`CREATE TABLE IF NOT EXISTS agents (
		id        INTEGER PRIMARY KEY AUTOINCREMENT,
		AgentName TEXT    NOT NULL UNIQUE,
		AgentId   INTEGER NOT NULL UNIQUE,
		LastSeen  INTEGER NOT NULL,
		CreatedAt INTEGER NOT NULL,
		IsAdmin   INTEGER NOT NULL,
		Username  TEXT    NOT NULL,
		Hostname  TEXT    NOT NULL,
		Domain    TEXT    NOT NULL,
		WinVer    TEXT    NOT NULL,
		IpAddr    TEXT    NOT NULL
	);`,
	// Status: paused, starting, running, stopping, stopped, error
	`CREATE TABLE IF NOT EXISTS listeners (
		id           INTEGER PRIMARY KEY AUTOINCREMENT,
		ListenerID   TEXT    NOT NULL UNIQUE,
		ListenerType TEXT    NOT NULL,
		ListenerName TEXT    NOT NULL UNIQUE,
		ListenerBind TEXT    NOT NULL,
		ListenerPort INTEGER NOT NULL,
		Status       TEXT    NOT NULL,
		CreatedAt    INTEGER NOT NULL,
		Hostname     TEXT    NOT NULL DEFAULT '',
		TlsCert      TEXT    NOT NULL DEFAULT '',
		TlsKey       TEXT    NOT NULL DEFAULT '',
		UNIQUE (ListenerBind, ListenerPort)
	);`,
	// Kind: path, header
	`CREATE TABLE IF NOT EXISTS http_listener_info (
		id         INTEGER PRIMARY KEY AUTOINCREMENT,
		ListenerID TEXT    NOT NULL,
		Kind       TEXT    NOT NULL,
		Value      TEXT    NOT NULL,
		Position   INTEGER NOT NULL DEFAULT 0,
		FOREIGN KEY (ListenerID) REFERENCES listeners (ListenerID) ON DELETE CASCADE
	);`,
	`CREATE INDEX IF NOT EXISTS idx_http_listener_info_listener
		ON http_listener_info (ListenerID, Position);`,
	`CREATE TABLE IF NOT EXISTS files (
		id        INTEGER PRIMARY KEY AUTOINCREMENT,
		FileID    TEXT    NOT NULL UNIQUE,
		AgentID   INTEGER NOT NULL,
		FileName  TEXT    NOT NULL,
		FilePath  TEXT    NOT NULL,
		Size      INTEGER NOT NULL,
		Sha256    TEXT    NOT NULL,
		CreatedAt INTEGER NOT NULL,
		FOREIGN KEY (AgentID) REFERENCES agents (id) ON DELETE CASCADE
	);`,
	// Status: 0 == pending, 1 == sent, 2 == completed, 3 == failed
	`CREATE TABLE IF NOT EXISTS tasks (
		id          INTEGER PRIMARY KEY AUTOINCREMENT,
		TaskID      INTEGER    NOT NULL UNIQUE,
		AgentID     INTEGER NOT NULL,
		CommandCode   INTEGER    NOT NULL,
		Args        TEXT NOT NULL CHECK (json_valid(Args)),
		Status      INTEGER NOT NULL,
		Output      TEXT,
		CreatedAt   INTEGER NOT NULL,
		CompletedAt INTEGER

	);`,
	//FOREIGN KEY (AgentID) REFERENCES agents (id) ON DELETE CASCADE
}

func openDB(path string) (*sql.DB, error) {
	return sql.Open("sqlite", "file:"+path+"?_pragma=foreign_keys(1)")
}

func SetupDB(dbPath string) error {
	conn, err := openDB(dbPath)
	if err != nil {
		return fmt.Errorf("open db: %w", err)
	}
	defer conn.Close()

	if err := conn.Ping(); err != nil {
		return fmt.Errorf("ping db: %w", err)
	}

	tx, err := conn.Begin()
	if err != nil {
		return fmt.Errorf("begin migration tx: %w", err)
	}
	defer tx.Rollback()

	for _, m := range migrations {
		if _, err := tx.Exec(m); err != nil {
			return fmt.Errorf("run migration: %w", err)
		}
	}

	if err := tx.Commit(); err != nil {
		return fmt.Errorf("commit migrations: %w", err)
	}

	return nil
}

type DB struct {
	conn *sql.DB
}

func New() (*DB, error) {
	home, err := os.UserHomeDir()
	if err != nil {
		return nil, err
	}

	c, err := openDB(filepath.Join(home, ".local", "share", "abyss", "database", "abyss.db"))
	if err != nil {
		return nil, err
	}

	return &DB{
		conn: c,
	}, nil
}
