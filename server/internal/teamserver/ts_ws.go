package teamserver

import "net/http"

func (ts *TS) HandleWS(w http.ResponseWriter, r *http.Request) {
	token := r.URL.Query().Get("token")
	if token == "" {
		ts.Error(w, http.StatusUnauthorized, "missing auth token")
		return
	}

	_, err := ts.Auth.Verify(token)
	if err != nil {
		ts.Error(w, http.StatusUnauthorized, "missing auth token")
		return
	}

	ts.WS.Upgrade(w, r)

}
