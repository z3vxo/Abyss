package cli

import (
	"context"
	"fmt"
	"io"
	"strconv"
	"strings"

	"github.com/spf13/pflag"
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

func (c *CLI) NewListener(args []string) error {
	fs := pflag.NewFlagSet("listeners new", pflag.ContinueOnError)
	fs.SetOutput(io.Discard)

	name := fs.StringP("name", "n", "", "listener name")
	bind := fs.StringP("bind", "b", "127.0.0.1", "address to bind to")
	port := fs.IntP("port", "p", 0, "port to listen on")
	proto := fs.StringP("type", "t", "http", "protocol to use (http or https)")
	host := fs.StringP("host", "h", "127.0.0.1", "hostname to use")
	cert := fs.StringP("cert", "c", "", "base64 encoded certificate")
	key := fs.StringP("key", "k", "", "base64 encoded key")
	startNow := fs.Bool("start", false, "start the listener now")

	if err := fs.Parse(args); err != nil {
		return err
	}
	if fs.NArg() > 0 {
		return fmt.Errorf("unexpected argument %q", fs.Arg(0))
	}
	if *name == "" {
		return fmt.Errorf("listener name is required")
	}
	if _, err := strconv.Atoi(*name); err == nil {
		return fmt.Errorf("listener name cannot be numeric")
	}
	if strings.HasPrefix(*name, "#") {
		return fmt.Errorf("listener name cannot start with '#'")
	}
	if *port == 0 {
		return fmt.Errorf("port is required")
	}
	if *proto != "http" && *proto != "https" {
		return fmt.Errorf("unsupported listener type %q", *proto)
	}

	req := NewListenerReq{
		Name:     *name,
		Type:     *proto,
		StartNow: *startNow,
		HttpInfo: HttpListenerInfo{
			Bind:      *bind,
			Port:      *port,
			Hostname:  *host,
			HttpPaths: []string{"/test"},
			TlsCert:   *cert,
			TlsKey:    *key,
		},
	}

	id, err := c.api.NewListener(context.Background(), req)
	if err != nil {
		return err
	}

	if *startNow {
		c.term.Infof("Created and started listener: %s", id)
	} else {
		c.term.Infof("Created listener: %s", id)
	}

	return nil
}
