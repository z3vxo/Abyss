package teamserver

import (
	"fmt"
	"net/http"

	"github.com/go-chi/chi"
	"github.com/z3vxo/Abyss/internal/agentserver"
	"github.com/z3vxo/Abyss/internal/auth"
	"github.com/z3vxo/Abyss/internal/config"
	"github.com/z3vxo/Abyss/internal/db"
	"github.com/z3vxo/Abyss/internal/listeners"
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

	agentSrv := &agentserver.AgentServerInfo{DB: database}

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
			r.Post("/listener/new", ts.HandleNewListener)
			r.Get("/listener/list", ts.HandleListListeners)

		})
	})

	if err := ts.startListeners(); err != nil {
		fmt.Printf("[!] Failed starting listeners: %v\n", err)
	}

	listen := fmt.Sprintf("%s:%d", ts.Cfg.TeamServer.BindAddr, ts.Cfg.TeamServer.Port)
	fmt.Printf("[*] Listening on %s\n", listen)
	return http.ListenAndServe(listen, r)
}
