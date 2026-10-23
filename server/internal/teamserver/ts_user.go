package teamserver

import (
	"encoding/json"
	"net/http"
)

type LoginReq struct {
	User string `json:"username"`
	Pass string `json:"password"`
}

func (ts *TS) HandleAuth(w http.ResponseWriter, r *http.Request) {
	var req LoginReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed decoding json")
		return
	}

	if ok := ts.Auth.CheckCreds(req.User, req.Pass); !ok {
		ts.Error(w, http.StatusUnauthorized, "invalid credentials")
		return
	}

	Tokens, err := ts.Auth.Generate(req.User)
	if err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed verifying creds")
		return
	}

	ts.Success(w, http.StatusOK, Tokens)
}

type RefreshReq struct {
	Refresh string `json:"refresh"`
}

func (ts *TS) HandleRefresh(w http.ResponseWriter, r *http.Request) {
	var req RefreshReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed decoding json")
		return
	}

	Cliams, err := ts.Auth.VerifyRefresh(req.Refresh)
	if err != nil {
		ts.Error(w, http.StatusInternalServerError, "failed validating token")
		return
	}

	Tokens, err := ts.Auth.Generate(Cliams.Username)
	ts.Success(w, http.StatusOK, Tokens)
}
