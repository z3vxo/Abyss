package teamserver

import (
	"encoding/json"
	"errors"
	"fmt"
	"net/http"

	"github.com/go-chi/chi"
	"github.com/z3vxo/Abyss/internal/db"
	"github.com/z3vxo/Abyss/internal/listeners"
	"github.com/z3vxo/Abyss/internal/shared"
)

// only http(s) for now

func (ts *TS) HandleNewListener(w http.ResponseWriter, r *http.Request) {
	r.Body = http.MaxBytesReader(w, r.Body, 1<<20)

	var req shared.NewListenerReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		ts.Error(w, http.StatusBadRequest, "invalid json")
		return
	}

	id, err := ts.CreateListener(req)
	if err != nil {
		ts.Error(w, http.StatusInternalServerError, err.Error())
		return
	}

	ts.Success(w, http.StatusCreated, map[string]string{"id": id})
}

func (ts *TS) HandleListListeners(w http.ResponseWriter, r *http.Request) {
	ts.Success(w, http.StatusOK, ts.ListenerMgr.List())
}

func (ts *TS) HandleStopListener(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	if id == "" {
		ts.Error(w, http.StatusBadRequest, "listener id must be present in url")
		return
	}

	if err := ts.ListenerMgr.Stop(id, nil); err != nil {
		ts.Error(w, http.StatusInternalServerError, err.Error())
		return
	}
	ts.Success(w, http.StatusOK, map[string]string{"stopped": id})
}

func (ts *TS) HandleDeleteListener(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	if id == "" {
		ts.Error(w, http.StatusBadRequest, "listener id must be present in url")
		return
	}

	if err := ts.ListenerMgr.Remove(id); err != nil {
		ts.Error(w, http.StatusInternalServerError, err.Error())
		return
	}

	ts.Success(w, http.StatusOK, map[string]string{"deleted": id})
}

func (ts *TS) HandleStartListener(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	if id == "" {
		ts.Error(w, http.StatusBadRequest, "listener id must be present in url")
		return
	}

	if err := ts.ListenerMgr.Start(id); err != nil {
		ts.Error(w, http.StatusInternalServerError, err.Error())
		return
	}

	ts.Success(w, http.StatusOK, map[string]string{"started": id})
}

func (ts *TS) CreateListener(req shared.NewListenerReq) (string, error) {
	id, err := ts.ListenerMgr.New(req)
	if err != nil {
		return "", err
	}

	record := db.ListenerRecord{
		ID:       id,
		Name:     req.Name,
		Type:     req.Type,
		Bind:     req.HttpInfo.Bind,
		Port:     req.HttpInfo.Port,
		Status:   string(listeners.StatusPaused),
		Hostname: req.HttpInfo.Hostname,
		TlsCert:  req.HttpInfo.TlsCert,
		TlsKey:   req.HttpInfo.TlsKey,
		Paths:    req.HttpInfo.HttpPaths,
		Headers:  req.HttpInfo.HttpResponseHeaders,
	}

	if err := ts.DB.InsertListener(record); err != nil {
		ts.ListenerMgr.Remove(id)
		return "", err
	}

	if req.StartNow {
		if err := ts.ListenerMgr.Start(id); err != nil {
			ts.DB.UpdateListenerStatus(id, string(listeners.StatusError))
			return id, err
		}
		if err := ts.DB.UpdateListenerStatus(id, string(listeners.StatusRunning)); err != nil {
			return id, err
		}
	}

	return id, nil
}

func (ts *TS) startListeners() error {
	records, err := ts.DB.GetListenersToStart()
	if err != nil {
		return err
	}

	var errs []error
	for _, record := range records {
		req := shared.NewListenerReq{
			Name:     record.Name,
			Type:     record.Type,
			StartNow: true,
			HttpInfo: shared.HttpListenerInfo{
				Bind:                record.Bind,
				Port:                record.Port,
				Hostname:            record.Hostname,
				HttpPaths:           record.Paths,
				HttpResponseHeaders: record.Headers,
				TlsCert:             record.TlsCert,
				TlsKey:              record.TlsKey,
			},
		}

		if err := ts.ListenerMgr.Restore(req, record.ID); err != nil {
			ts.DB.UpdateListenerStatus(record.ID, string(listeners.StatusError))
			errs = append(errs, fmt.Errorf("restore listener %q: %w", record.ID, err))
			continue
		}

		if err := ts.ListenerMgr.Start(record.ID); err != nil {
			ts.DB.UpdateListenerStatus(record.ID, string(listeners.StatusError))
			errs = append(errs, fmt.Errorf("start listener %q: %w", record.ID, err))
			continue
		}

		if err := ts.DB.UpdateListenerStatus(record.ID, string(listeners.StatusRunning)); err != nil {
			errs = append(errs, fmt.Errorf("update listener %q status: %w", record.ID, err))
		}
	}

	return errors.Join(errs...)
}
