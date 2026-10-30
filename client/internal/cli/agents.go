package cli

import (
	"context"
	"fmt"
	"strconv"
	"strings"

	"github.com/z3vxo/Abyss/client/internal/client"
	"github.com/z3vxo/Abyss/client/internal/render"
)

func (c *CLI) ListAgents(args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("agents list takes no arguments")
	}

	agents, err := c.api.ListAgents(context.Background())
	if err != nil {
		return err
	}

	c.agents = agents.Agents
	c.term.Printf("%s\n", render.RenderAgents(agents))

	return nil
}

func (c *CLI) IsUsingAgent() bool {
	if c.AgentInUse == 0 {
		c.term.Warnf("must be using agents")
		return false
	}

	return true
}

func (c *CLI) UseAgent(args []string) error {
	if len(args) != 1 {
		return fmt.Errorf("usage: use <name|id|index>")
	}

	agent, err := c.resolveAgent(args[0])
	if err != nil {
		return err
	}

	c.AgentInUse = int(agent.AgentId)
	c.term.SetPrompt(fmt.Sprintf("Abyss(%s) $> ", agent.AgentName))
	c.term.Infof("Using agent %s (%d)", agent.AgentName, agent.AgentId)

	return nil
}

func (c *CLI) resolveAgent(ref string) (client.Agent, error) {
	if ref == "" {
		return client.Agent{}, fmt.Errorf("must provide agent name, id, or index")
	}

	ref = strings.TrimPrefix(ref, "#")

	if index, err := strconv.Atoi(ref); err == nil {
		if len(c.agents) == 0 {
			return client.Agent{}, fmt.Errorf("no agents loaded, run `agents list` first")
		}
		if index < 1 || index > len(c.agents) {
			return client.Agent{}, fmt.Errorf("agent index %d out of range (1-%d)", index, len(c.agents))
		}
		return c.agents[index-1], nil
	}

	if agent, ok := c.findAgent(ref); ok {
		return agent, nil
	}

	agents, err := c.api.ListAgents(context.Background())
	if err != nil {
		return client.Agent{}, err
	}
	c.agents = agents.Agents

	if agent, ok := c.findAgent(ref); ok {
		return agent, nil
	}

	return client.Agent{}, fmt.Errorf("agent %q not found", ref)
}

func (c *CLI) findAgent(ref string) (client.Agent, bool) {
	for _, agent := range c.agents {
		if agent.AgentName == ref || strconv.FormatInt(int64(agent.AgentId), 10) == ref {
			return agent, true
		}
	}

	return client.Agent{}, false
}
