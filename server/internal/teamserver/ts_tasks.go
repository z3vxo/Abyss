package teamserver

import (
	"encoding/json"
	"net/http"

	"github.com/z3vxo/Abyss/internal/shared"
)

func (ts *TS) HandleNewTask(w http.ResponseWriter, r *http.Request) {
	var req shared.NewTaskReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed decoding json")
		return
	}

	id, err := ts.DB.InsertTask(req)
	if err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed inserting task")
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
