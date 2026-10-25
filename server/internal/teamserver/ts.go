package teamserver

import (
	"fmt"
	"net/http"

	"github.com/go-chi/chi"
	"github.com/z3vxo/Abyss/server/internal/agentserver/httptransport"
	"github.com/z3vxo/Abyss/server/internal/auth"
	"github.com/z3vxo/Abyss/server/internal/config"
	"github.com/z3vxo/Abyss/server/internal/db"
	"github.com/z3vxo/Abyss/server/internal/listeners"
)

type TS struct {
	DB          *db.DB
	Cfg         *config.Config
	Auth        *auth.Auth
	ListenerMgr *listeners.Listeners
}

func New(cfg *config.Config) (*TS, error) {
	database, err := db.New()
	if err != nil {
		return nil, err
	}

	agentSrv := &httptransport.AgentServerInfo{DB: database}

	return &TS{
		DB:          database,
		Cfg:         cfg,
		Auth:        auth.New(cfg),
		ListenerMgr: listeners.Create(agentSrv),
	}, nil

}

func (ts *TS) Run() error {
	r := chi.NewRouter()

	r.Route("/rest", func(r chi.Router) {
		r.Post("/auth/login", ts.HandleAuth)
		r.Post("/auth/refresh", ts.HandleRefresh)
		//r.Get("/ws/connect", ts.HandleWS)

		r.Group(func(r chi.Router) {
			r.Use(ts.Auth.AuthWare())
			//-------
			r.Delete("/listener/delete/{id}", ts.HandleDeleteListener)
			r.Get("/listener/list", ts.HandleListListeners)
			r.Post("/listener/new", ts.HandleNewListener)
			r.Post("/listener/start/{id}", ts.HandleStartListener)
			r.Patch("/listener/stop/{id}", ts.HandleStopListener)
			//-------
			r.Post("/tasks/new", ts.HandleNewTask)
			r.Post("/tasks/delete", ts.HandleDeleteTask)
			r.Post("/tasks/list", ts.HandleListTasks)
			//-----
			r.Post("/files/begin", ts.HandleBeginFile)
			r.Put("/files/upload/{uuid}", ts.HandleUploadFile)
			r.Post("/files/list", ts.HandleListFiles)
			r.Get("/files/download/{uuid}", ts.HandleDownloadFile)

		})
	})

	if err := ts.startListeners(); err != nil {
		fmt.Printf("[!] Failed starting listeners: %v\n", err)
	}

	listen := fmt.Sprintf("%s:%d", ts.Cfg.TeamServer.BindAddr, ts.Cfg.TeamServer.Port)
	fmt.Printf("[*] Listening on %s\n", listen)
	return http.ListenAndServe(listen, r)
}
