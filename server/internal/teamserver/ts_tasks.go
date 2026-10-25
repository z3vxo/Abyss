package teamserver

import (
	"encoding/json"
	"net/http"

	"github.com/z3vxo/Abyss/server/internal/shared"
)

func (ts *TS) HandleNewTask(w http.ResponseWriter, r *http.Request) {
	var req shared.NewTaskReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed decoding json")
		return
	}

	id, err := ts.DB.InsertTask(req)
	if err != nil {
		ts.Error(w, http.StatusBadRequest, err.Error())
		return
	}

	ts.Success(w, http.StatusOK, map[string]int32{"task_id": id})

}

type DeleteReq struct {
	TaskID int32 `json:"task_id"`
}

func (ts *TS) HandleDeleteTask(w http.ResponseWriter, r *http.Request) {

	var req DeleteReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed decoding json")
		return
	}

	if err := ts.DB.DeleteTask(req.TaskID); err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed deleting task")
		return
	}

	ts.Success(w, http.StatusOK, map[string]int32{"deleted task": req.TaskID})

}

type ListReq struct {
	AgentID int32 `json:"agent_id"`
}

func (ts *TS) HandleListTasks(w http.ResponseWriter, r *http.Request) {
	var req ListReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed decoding json")
		return
	}

	Tasks, err := ts.DB.ListTasks(req.AgentID)
	if err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed listing tasks")
		return
	}

	ts.Success(w, http.StatusOK, Tasks)
}
