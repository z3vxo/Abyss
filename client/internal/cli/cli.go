package cli

import (
	"errors"
	"io"
	"strings"

	"github.com/z3vxo/Abyss/client/internal/client"
	"github.com/z3vxo/Abyss/client/internal/config"
	"github.com/z3vxo/Abyss/client/internal/term"
)

type CLI struct {
	Cfg        *config.Config
	term       *term.Console
	api        *client.Client
	listeners  []client.ListenerInfo
	Commands   map[string]CommandFunc
	AgentInUse int
}

func New(configPath string, debug bool) (*CLI, error) {
	cfg, err := config.New(configPath, debug)
	if err != nil {
		return nil, err
	}

	apiClient, err := client.New(cfg)
	if err != nil {
		return nil, err
	}

	console, err := term.New("Abyss $> ")
	if err != nil {
		return nil, err
	}

	c := &CLI{
		Cfg:  cfg,
		term: console,
		api:  apiClient,
	}

	c.Commands = map[string]CommandFunc{
		"listeners": c.HandleListeners,
		"mv":        c.HandleMv,
		"whoami":    c.HandleWhoami,
	}

	return c, nil
}

func (c *CLI) Run() error {
	defer c.term.Close()

	for {
		line, err := c.term.Readline()
		if err != nil {
			if errors.Is(err, io.EOF) || term.IsInterrupt(err) {
				return nil
			}
			return err
		}

		if strings.TrimSpace(line) == "" {
			continue
		}

		if err := c.Parse(line); err != nil {
			c.term.Errorf("%v", err)
		}
	}
}
