package cli

import (
	"context"
	"fmt"
	"net"
	"strconv"
	"strings"

	"github.com/z3vxo/Abyss/client/internal/client"
	"github.com/z3vxo/Abyss/client/internal/render"
)

func (c *CLI) ListListener(args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("listeners list takes no arguments")
	}

	listeners, err := c.api.ListListeners(context.Background())
	if err != nil {
		return err
	}

	c.listeners = listeners
	c.term.Printf("%s\n", render.Listeners(listeners))

	return nil
}

func (c *CLI) StartListener(args []string) error {
	return c.listenerAction("start", args)
}

func (c *CLI) StopListener(args []string) error {
	return c.listenerAction("stop", args)
}

func (c *CLI) DeleteListener(args []string) error {
	return c.listenerAction("delete", args)
}

func (c *CLI) listenerAction(action string, args []string) error {
	if len(args) != 1 {
		return fmt.Errorf("usage: listeners %s <name|id|index>", action)
	}

	listener, err := c.resolveListener(args[0])
	if err != nil {
		return err
	}

	ctx := context.Background()
	switch action {
	case "start":
		if err := c.api.StartListener(ctx, listener.ID); err != nil {
			return err
		}
		c.term.Infof("Started listener %q", listener.Name)
	case "stop":
		if err := c.api.StopListener(ctx, listener.ID); err != nil {
			return err
		}
		c.term.Infof("Stopped listener %q", listener.Name)
	case "delete":
		if err := c.api.DeleteListener(ctx, listener.ID); err != nil {
			return err
		}
		c.term.Infof("Deleted listener %q", listener.Name)
	}

	return nil
}

func (c *CLI) resolveListener(ref string) (client.ListenerInfo, error) {
	if ref == "" {
		return client.ListenerInfo{}, fmt.Errorf("must provide listener name, id, or index")
	}

	ref = strings.TrimPrefix(ref, "#")

	if index, err := strconv.Atoi(ref); err == nil {
		if len(c.listeners) == 0 {
			return client.ListenerInfo{}, fmt.Errorf("no listeners loaded, run `listeners list` first")
		}
		if index < 1 || index > len(c.listeners) {
			return client.ListenerInfo{}, fmt.Errorf("listener index %d out of range (1-%d)", index, len(c.listeners))
		}
		return c.listeners[index-1], nil
	}

	if listener, ok := c.findListener(ref); ok {
		return listener, nil
	}

	listeners, err := c.api.ListListeners(context.Background())
	if err != nil {
		return client.ListenerInfo{}, err
	}
	c.listeners = listeners

	if listener, ok := c.findListener(ref); ok {
		return listener, nil
	}

	return client.ListenerInfo{}, fmt.Errorf("listener %q not found", ref)
}

func (c *CLI) findListener(ref string) (client.ListenerInfo, bool) {
	for _, listener := range c.listeners {
		if listener.Name == ref || listener.ID == ref {
			return listener, true
		}
	}

	return client.ListenerInfo{}, false
}

func isWildcardBind(bind string) bool {
	if bind == "" {
		return true
	}

	ip := net.ParseIP(strings.Trim(bind, "[]"))
	return ip != nil && ip.IsUnspecified()
}

func (c *CLI) NewListener(args []string) error {
	if len(args) > 2 {
		return fmt.Errorf("usage: listeners new [profile] [name]")
	}

	profileName := c.Cfg.Defaults.Listener
	name := ""
	switch len(args) {
	case 1:
		profileName = args[0]
	case 2:
		profileName, name = args[0], args[1]
	}

	if profileName == "" {
		return fmt.Errorf("no listener profile provided and no default configured")
	}

	profile, err := c.Cfg.Listener(profileName)
	if err != nil {
		return err
	}

	if name == "" {
		name = profileName
	}
	if _, err := strconv.Atoi(name); err == nil {
		return fmt.Errorf("listener name cannot be numeric")
	}
	if strings.HasPrefix(name, "#") {
		return fmt.Errorf("listener name cannot start with '#'")
	}

	hostname := profile.Hostname
	if hostname == "" && !isWildcardBind(profile.Bind) {
		hostname = profile.Bind
	}

	req := NewListenerReq{
		Name:     name,
		Type:     profile.Type,
		StartNow: profile.StartNow,
		HttpInfo: HttpListenerInfo{
			Bind:                profile.Bind,
			Port:                profile.Port,
			Hostname:            hostname,
			HttpPaths:           profile.Paths,
			HttpResponseHeaders: profile.ResponseHeaders,
			TlsCert:             profile.TLSCert,
			TlsKey:              profile.TLSKey,
		},
	}

	id, err := c.api.NewListener(context.Background(), req)
	if err != nil {
		return err
	}

	if profile.StartNow {
		c.term.Infof("Created and started listener %q from profile %q: %s", name, profileName, id)
	} else {
		c.term.Infof("Created listener %q from profile %q: %s", name, profileName, id)
	}

	return nil
}

func (c *CLI) ListProfiles(args []string) error {
	if len(args) > 0 {
		return fmt.Errorf("listeners profiles takes no arguments")
	}

	names := c.Cfg.ListenerNames()
	if len(names) == 0 {
		c.term.Printf("No listener profiles.\n")
		return nil
	}

	for _, name := range names {
		profile, err := c.Cfg.Listener(name)
		if err != nil {
			c.term.Printf("%s  invalid: %v\n", name, err)
			continue
		}

		defaultMark := ""
		if name == c.Cfg.Defaults.Listener {
			defaultMark = " (default)"
		}

		c.term.Printf("%s%s  %s  %s:%d  paths=%s  start_now=%t\n",
			name, defaultMark, profile.Type, profile.Bind, profile.Port,
			strings.Join(profile.Paths, ","), profile.StartNow)
	}

	return nil
}
