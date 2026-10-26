package term

import (
	"errors"
	"fmt"
	"os"
	"sync"

	"github.com/chzyer/readline"
)

var ErrInterrupt = readline.ErrInterrupt

const (
	colorRed    = "\033[31m"
	colorYellow = "\033[33m"
	colorGreen  = "\033[32m"
	colorReset  = "\033[0m"
)

type Console struct {
	rl    *readline.Instance
	mu    sync.Mutex
	color bool
}

func New(prompt string) (*Console, error) {
	rl, err := readline.NewEx(&readline.Config{
		Prompt: prompt,
	})
	if err != nil {
		return nil, err
	}

	return &Console{
		rl:    rl,
		color: readline.IsTerminal(int(os.Stdout.Fd())),
	}, nil
}

func (c *Console) Readline() (string, error) {
	return c.rl.Readline()
}

func (c *Console) Write(p []byte) (int, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	c.rl.Clean()
	n, err := c.rl.Stdout().Write(p)
	c.rl.Refresh()

	return n, err
}

func (c *Console) Printf(format string, args ...any) {
	c.mu.Lock()
	defer c.mu.Unlock()

	c.rl.Clean()
	fmt.Fprintf(c.rl.Stdout(), format, args...)
	c.rl.Refresh()
}

func (c *Console) Errorf(format string, args ...any) {
	c.printTag(colorRed, "[!]", format, args...)
}

func (c *Console) Warnf(format string, args ...any) {
	c.printTag(colorYellow, "[~]", format, args...)
}

func (c *Console) Infof(format string, args ...any) {
	c.printTag(colorGreen, "[*]", format, args...)
}

func (c *Console) Successf(format string, args ...any) {
	c.printTag(colorGreen, "[+]", format, args...)
}

func (c *Console) printTag(color, tag, format string, args ...any) {
	c.printLine(c.colorize(color, tag) + " " + fmt.Sprintf(format, args...))
}

func (c *Console) printLine(line string) {
	c.mu.Lock()
	defer c.mu.Unlock()

	c.rl.Clean()
	fmt.Fprintln(c.rl.Stdout(), line)
	c.rl.Refresh()
}

func (c *Console) colorize(code, s string) string {
	if !c.color {
		return s
	}
	return code + s + colorReset
}

func (c *Console) Clear() {
	c.mu.Lock()
	defer c.mu.Unlock()

	fmt.Fprint(c.rl.Stdout(), "\033[H\033[2J")
	c.rl.Refresh()
}

func (c *Console) SetPrompt(prompt string) {
	c.mu.Lock()
	defer c.mu.Unlock()

	c.rl.SetPrompt(prompt)
}

func (c *Console) Close() error {
	c.mu.Lock()
	defer c.mu.Unlock()

	return c.rl.Close()
}

func IsInterrupt(err error) bool {
	return errors.Is(err, ErrInterrupt)
}
