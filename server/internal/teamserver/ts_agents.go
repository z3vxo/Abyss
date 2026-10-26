package teamserver

import "net/http"

func (ts *TS) HandleListAgents(w http.ResponseWriter, r *http.Request) {

	agents, err := ts.DB.ListAgents()
	if err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed retreiving agents")
		return
	}

	ts.Success(w, http.StatusOK, agents)

}
