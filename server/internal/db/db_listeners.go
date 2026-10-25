package db

import (
	"database/sql"
	"fmt"
	"time"
)

type ListenerRecord struct {
	ID       string
	Name     string
	Type     string
	Bind     string
	Port     int
	Status   string
	Hostname string
	TlsCert  string
	TlsKey   string
	Paths    []string
	Headers  []string
}

func (d *DB) InsertListener(rec ListenerRecord) error {
	tx, err := d.conn.Begin()
	if err != nil {
		return fmt.Errorf("begin listener insert: %w", err)
	}
	defer tx.Rollback()

	if _, err := tx.Exec(`INSERT INTO listeners
		(ListenerID, ListenerType, ListenerName, ListenerBind, ListenerPort, Status, CreatedAt, Hostname, TlsCert, TlsKey)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
		rec.ID, rec.Type, rec.Name, rec.Bind, rec.Port, rec.Status, time.Now().Unix(),
		rec.Hostname, rec.TlsCert, rec.TlsKey); err != nil {
		return fmt.Errorf("insert listener %s: %w", rec.ID, err)
	}

	if err := insertListenerInfo(tx, rec.ID, "path", rec.Paths); err != nil {
		return err
	}
	if err := insertListenerInfo(tx, rec.ID, "header", rec.Headers); err != nil {
		return err
	}

	if err := tx.Commit(); err != nil {
		return fmt.Errorf("commit listener insert: %w", err)
	}
	return nil
}

func insertListenerInfo(tx *sql.Tx, id, kind string, values []string) error {
	for pos, value := range values {
		if _, err := tx.Exec(`INSERT INTO http_listener_info (ListenerID, Kind, Value, Position)
			VALUES (?, ?, ?, ?)`, id, kind, value, pos); err != nil {
			return fmt.Errorf("insert listener %s %s info: %w", id, kind, err)
		}
	}
	return nil
}

func (d *DB) DeleteListener(id string) error {
	res, err := d.conn.Exec(`DELETE FROM listeners WHERE ListenerID = ?`, id)
	if err != nil {
		return fmt.Errorf("delete listener %s: %w", id, err)
	}

	n, err := res.RowsAffected()
	if err != nil {
		return fmt.Errorf("delete listener %s: %w", id, err)
	}
	if n == 0 {
		return fmt.Errorf("listener %s not found", id)
	}

	return nil
}

func (d *DB) UpdateListenerStatus(id, status string) error {
	if _, err := d.conn.Exec(`UPDATE listeners SET Status = ? WHERE ListenerID = ?`, status, id); err != nil {
		return fmt.Errorf("update listener %s status: %w", id, err)
	}
	return nil
}

func (d *DB) ListListeners() ([]ListenerRecord, error) {
	return d.listListeners("")
}

func (d *DB) GetListenersToStart() ([]ListenerRecord, error) {
	return d.listListeners("WHERE Status IN ('running', 'starting')")
}

func (d *DB) listListeners(where string) ([]ListenerRecord, error) {
	rows, err := d.conn.Query(`SELECT ListenerID, ListenerType, ListenerName, ListenerBind,
		ListenerPort, Status, Hostname, TlsCert, TlsKey FROM listeners ` + where + ` ORDER BY CreatedAt`)
	if err != nil {
		return nil, fmt.Errorf("query listeners: %w", err)
	}

	var records []ListenerRecord
	index := make(map[string]int)

	for rows.Next() {
		var rec ListenerRecord
		if err := rows.Scan(&rec.ID, &rec.Type, &rec.Name, &rec.Bind, &rec.Port, &rec.Status,
			&rec.Hostname, &rec.TlsCert, &rec.TlsKey); err != nil {
			rows.Close()
			return nil, fmt.Errorf("scan listener: %w", err)
		}
		index[rec.ID] = len(records)
		records = append(records, rec)
	}
	if err := rows.Err(); err != nil {
		rows.Close()
		return nil, fmt.Errorf("iterate listeners: %w", err)
	}
	rows.Close()

	if len(records) == 0 {
		return records, nil
	}

	infoRows, err := d.conn.Query(`SELECT ListenerID, Kind, Value
		FROM http_listener_info ORDER BY ListenerID, Position`)
	if err != nil {
		return nil, fmt.Errorf("query listener info: %w", err)
	}
	defer infoRows.Close()

	for infoRows.Next() {
		var id, kind, value string
		if err := infoRows.Scan(&id, &kind, &value); err != nil {
			return nil, fmt.Errorf("scan listener info: %w", err)
		}
		i, ok := index[id]
		if !ok {
			continue
		}
		switch kind {
		case "path":
			records[i].Paths = append(records[i].Paths, value)
		case "header":
			records[i].Headers = append(records[i].Headers, value)
		}
	}
	if err := infoRows.Err(); err != nil {
		return nil, fmt.Errorf("iterate listener info: %w", err)
	}

	return records, nil
}
