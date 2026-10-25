package db

import (
	"encoding/json"
	"fmt"
	"time"

	"github.com/z3vxo/Abyss/server/internal/shared"
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

var CommandToCodeMap = map[string]int{
	"whoami": 0x1,
	"cat":    0x2,
	"mv":     0x3,
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
	code, ok := CommandToCodeMap[task.Command]
	if !ok {
		return 0, fmt.Errorf("invalid command name")
	}

	q := `INSERT INTO tasks(TaskID, AgentID, CommandCode, Args, Status, CreatedAt) VALUES(?,?,?,?,?,?)`

	argsJSON, err := json.Marshal(task.Args)
	if err != nil {
		return 0, fmt.Errorf("marshal task args: %w", err)
	}

	TaskID, err := shared.GenRandomInt32()
	if err != nil {
		return 0, fmt.Errorf("generate task id: %w", err)
	}

	if _, err := db.conn.Exec(q, TaskID, task.AgentId, code, argsJSON, "queued", time.Now().Unix()); err != nil {
		return 0, fmt.Errorf("insert task: %w", err)
	}

	return TaskID, nil
}

func (db *DB) DeleteTask(id int32) error {
	q := `DELETE FROM tasks WHERE TaskID = ?`
	_, err := db.conn.Exec(q, id)
	return err
}
