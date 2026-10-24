package db

import (
	"encoding/json"
	"fmt"
	"time"

	"github.com/z3vxo/Abyss/internal/shared"
)

func (db *DB) InsertTask(task shared.NewTaskReq) (int32, error) {

	q := `INSERT INTO tasks(TaskID, AgentID, CommandCode, Args, Status, CreatedAt) VALUES(?,?,?,?,?,?)`

	argsJSON, err := json.Marshal(task.Args)
	if err != nil {
		fmt.Println(err)
		return 0, err
	}

	TaskID, err := shared.GenRandomInt32()
	if err != nil {
		fmt.Println(err)
		return 0, err
	}

	_, err = db.conn.Exec(q, TaskID, task.AgentId, task.CommandId, argsJSON, "queued", time.Now().Unix())
	fmt.Println(err)

	return TaskID, err
}

func (db *DB) DeleteTask(id int32) error {
	q := `DELETE FROM tasks WHERE TaskID = ?`
	_, err := db.conn.Exec(q, id)
	return err
}
