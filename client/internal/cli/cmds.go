package cli

import (
	"context"
	"fmt"
)

func (c *CLI) HandleWhoami(args []string) error {

	var req NewTaskReq
	req.AgentId = c.AgentInUse
	req.Command = "whoami"
	req.Args = []Arg{}

	msg, err := c.api.NewTask(context.Background(), req)
	if err != nil {
		return err
	}

	c.term.Printf("[*] Task Issued: %d\n", msg)

	return nil
}

func (c *CLI) HandleMv(args []string) error {
	if len(args) != 2 {
		return fmt.Errorf("usage: mv <src> <dst>")
	}

	req := NewTaskReq{
		AgentId: c.AgentInUse,
		Command: "mv",
		Args: []Arg{
			{Type: "string", Arg: args[0]},
			{Type: "string", Arg: args[1]},
		},
	}

	id, err := c.api.NewTask(context.Background(), req)
	if err != nil {
		return err
	}

	c.term.Printf("[*] Task Issued: %d\n", id)

	return nil
}
