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
			c.term.Infof("Agent called back, sent %d bytes\n", body.TotalSize)
			for _, out := range body.Output {
				c.term.Infof("Output for %d\n\n%s\n", out.TaskID, out.Output)
			}
		}
	}
}
