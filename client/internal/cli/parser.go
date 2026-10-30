package cli

import (
	"errors"
	"fmt"
	"strings"

	"github.com/google/shlex"
)

var ErrNoCommand = errors.New("no command provided")

type CommandFunc func(args []string) error

func (c *CLI) HandleAgents(args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("must provide a subcommand: list")
	}

	switch args[0] {
	case "list":
		return c.ListAgents(args[1:])
	default:
		return fmt.Errorf("unknown agents subcommand %q", args[0])
	}
}

func (c *CLI) HandleListeners(args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("must provide a subcommand: new, list, start, stop, delete, profiles")
	}

	switch args[0] {
	case "list":
		return c.ListListener(args[1:])
	case "new":
		return c.NewListener(args[1:])
	case "profiles":
		return c.ListProfiles(args[1:])
	case "start":
		return c.StartListener(args[1:])
	case "stop":
		return c.StopListener(args[1:])
	case "delete":
		return c.DeleteListener(args[1:])
	default:
		return fmt.Errorf("unknown listeners subcommand %q", args[0])
	}
}

func (c *CLI) Parse(input string) error {
	values, err := shlex.Split(input)
	if err != nil {
		return fmt.Errorf("parse input: %w", err)
	}

	if len(values) == 0 {
		return ErrNoCommand
	}

	command := strings.ToLower(values[0])

	cmd, ok := c.Commands[command]
	if !ok {
		return fmt.Errorf("unknown command %q", values[0])
	}

	return cmd(values[1:])
}
