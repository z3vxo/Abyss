package render

import (
	"github.com/jedib0t/go-pretty/v6/table"
	"github.com/z3vxo/Abyss/client/internal/client"
)

func RenderAgents(agents client.AgentsResp) string {
	if len(agents.Agents) == 0 {
		return "No agents."
	}

	w := table.NewWriter()
	w.SetStyle(table.StyleRounded)
	w.AppendHeader(table.Row{"#", "NAME", "HOST", "USER", "DOMAIN", "OS", "ARCH", "ADMIN", "PID"})

	for i, agent := range agents.Agents {
		w.AppendRow(table.Row{
			i + 1,
			agent.AgentName,
			agent.Hostname,
			agent.Username,
			agent.Domain,
			agent.WinVer,
			agent.Arch,
			agent.IsAdmin,
			agent.PID,
		})
	}

	return w.Render()
}

func Listeners(listeners []client.ListenerInfo) string {
	if len(listeners) == 0 {
		return "No listeners."
	}

	w := table.NewWriter()
	w.SetStyle(table.StyleRounded)
	w.AppendHeader(table.Row{"#", "NAME", "TYPE", "BIND", "PORT", "STATUS"})

	for i, listener := range listeners {
		w.AppendRow(table.Row{
			i + 1,
			listener.Name,
			listener.Type,
			listener.Bind,
			listener.Port,
			listener.Status,
		})
	}

	return w.Render()
}
