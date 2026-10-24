package db

import (
	"encoding/json"
	"fmt"
	"time"

	"github.com/z3vxo/Abyss/internal/shared"
)

type Task struct {
	TaskId      int32        `json:"task_id"`
	CommandCode int          `json:"command_code"`
	Args        []shared.Arg `json:"args"`
	Status      string       `json:"status"`
	CreatedAt   int32        `json:"created_at"`
}

type TasksResp struct {
	Total int    `json:"total"`
	Tasks []Task `json:"tasks"`
}

func (db *DB) ListTasks(id int32) (*TasksResp, error) {
	q := `SELECT TaskID, CommandCode, Args, Status, CreatedAt FROM tasks WHERE AgentID = ?`

	rows, err := db.conn.Query(q, id)

	if err != nil {

		return &TasksResp{}, err
	}

	var tasks []Task
	for rows.Next() {
		var t Task
		var argsRaw []byte
		if err := rows.Scan(&t.TaskId, &t.CommandCode, &argsRaw, &t.Status, &t.CreatedAt); err != nil {
			return &TasksResp{}, err
		}

		if err := json.Unmarshal(argsRaw, &t.Args); err != nil {

			return &TasksResp{}, err
		}
		tasks = append(tasks, t)
	}

	if err = rows.Err(); err != nil {
		fmt.Println(err)

		return &TasksResp{}, err
	}

	return &TasksResp{Total: len(tasks), Tasks: tasks}, nil
}

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
