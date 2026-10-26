package cli

import "github.com/z3vxo/Abyss/client/internal/ws"

func (c *CLI) handleEvents() {
	for event := range c.api.Events {
		switch event.Event {
		case ws.EventNewAgent:
			var body ws.NewAgent
			if err := event.Decode(&body); err != nil {
				continue
			}
			c.term.Infof("New agent %s (%s\\%s) connected", body.AgentName, body.Hostname, body.Username)
		case ws.EventTaskOutput:
			var body ws.TaskOutput
			if err := event.Decode(&body); err != nil {
				continue
			}
			c.term.Printf("[*] Task %d output from agent %d (%d bytes):\n%s\n", body.TaskID, body.AgentID, body.TotalPostSize, body.Output)
		}
	}
}
