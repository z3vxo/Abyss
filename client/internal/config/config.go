package config

import (
	"fmt"
	"os"
	"sort"
	"strings"

	"gopkg.in/yaml.v3"
)

type Connection struct {
	Host     string `yaml:"host"`
	Port     int    `yaml:"port"`
	Username string `yaml:"username"`
	Password string `yaml:"password"`
}

type Logging struct {
	Level   string `yaml:"level"`
	LogFile string `yaml:"logfile"`
}

type ListenerProfile struct {
	Type            string   `yaml:"type"`
	Bind            string   `yaml:"bind"`
	Port            int      `yaml:"port"`
	Hostname        string   `yaml:"hostname"`
	Paths           []string `yaml:"paths"`
	ResponseHeaders []string `yaml:"response_headers"`
	TLSCert         string   `yaml:"tls_cert"`
	TLSKey          string   `yaml:"tls_key"`
	StartNow        bool     `yaml:"start_now"`
}

type Defaults struct {
	Listener string `yaml:"listener"`
}

type Config struct {
	ConnectionCfg Connection                 `yaml:"connection"`
	LoggingCfg    Logging                    `yaml:"logging"`
	Defaults      Defaults                   `yaml:"defaults"`
	Listeners     map[string]ListenerProfile `yaml:"listeners"`
	Debug         bool                       `yaml:"-"`
}

func New(cfgPath string, debug bool) (*Config, error) {
	data, err := os.ReadFile(cfgPath)
	if err != nil {
		return nil, err
	}
	var cfg Config
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return nil, err
	}
	cfg.Debug = debug

	if cfg.Defaults.Listener != "" {
		if _, ok := cfg.Listeners[cfg.Defaults.Listener]; !ok {
			return nil, fmt.Errorf("default listener profile %q not found", cfg.Defaults.Listener)
		}
	}

	return &cfg, nil
}

func (cfg *Config) Listener(name string) (ListenerProfile, error) {
	profile, ok := cfg.Listeners[name]
	if !ok {
		return ListenerProfile{}, fmt.Errorf("unknown listener profile %q", name)
	}

	if profile.Type == "" {
		profile.Type = "http"
	}
	if profile.Type != "http" && profile.Type != "https" {
		return ListenerProfile{}, fmt.Errorf("listener profile %q: unsupported type %q", name, profile.Type)
	}

	if profile.Bind == "" {
		profile.Bind = "0.0.0.0"
	}
	if profile.Port == 0 {
		return ListenerProfile{}, fmt.Errorf("listener profile %q: port is required", name)
	}
	if len(profile.Paths) == 0 {
		return ListenerProfile{}, fmt.Errorf("listener profile %q: at least one path is required", name)
	}
	for _, path := range profile.Paths {
		if !strings.HasPrefix(path, "/") {
			return ListenerProfile{}, fmt.Errorf("listener profile %q: path %q must start with /", name, path)
		}
	}

	return profile, nil
}

func (cfg *Config) ListenerNames() []string {
	names := make([]string, 0, len(cfg.Listeners))
	for name := range cfg.Listeners {
		names = append(names, name)
	}
	sort.Strings(names)
	return names
}
