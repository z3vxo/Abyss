package cli

import (
	"context"
	"fmt"
)

func (c *CLI) HandleCat(args []string) error {
	if !c.IsUsingAgent() {
		return nil
	}
	if len(args) != 1 {
		return fmt.Errorf("usage: cat <filename>")
	}


	req := NewTaskReq{
		AgentId: c.AgentInUse,
		Command: "cat",
		Args: []Arg{
			{Type: "string", Arg: args[0]},
		},
	}
	id, err := c.api.NewTask(context.Background(), req)
	if err != nil {
		return err
	}

	c.term.Printf("[*] Task Issued: %d\n", id)
	return nil
}

func (c *CLI) HandleCd(args []string) error {
	if !c.IsUsingAgent() {
		return nil
	}
	if len(args) != 1 {
		return fmt.Errorf("usage: cd <dir>")
	}


	req := NewTaskReq{
		AgentId: c.AgentInUse,
		Command: "cd",
		Args: []Arg{
			{Type: "string", Arg: args[0]},
		},
	}
	id, err := c.api.NewTask(context.Background(), req)
	if err != nil {
		return err
	}

	c.term.Printf("[*] Task Issued: %d\n", id)
	return nil


}

func (c *CLI) HandleWhoami(args []string) error {
	if !c.IsUsingAgent() {
		return nil
	}

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
	if !c.IsUsingAgent() {
		return nil
	}
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
