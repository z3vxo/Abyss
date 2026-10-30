package shared

import (
	"crypto/rand"
	"encoding/binary"
	"math"
)

func GenRandomInt32() (int32, error) {
	var buf [4]byte
	if _, err := rand.Read(buf[:]); err != nil {
		return 0, err
	}
	const min int64 = 1_500_000_000
	const max int64 = math.MaxInt32

	n := int64(binary.BigEndian.Uint32(buf[:]) & math.MaxInt32)
	result := min + n&(max-min+1)
	return int32(result), nil
}

type HttpListenerInfo struct {
	Bind                string   `json:"bind_addr" yaml:"bind_addr"`
	Port                int      `json:"port" yaml:"port"`
	Hostname            string   `json:"hostname" yaml:"hostname"`
	HttpPaths           []string `json:"http_paths" yaml:"http_paths"`
	HttpResponseHeaders []string `json:"http_response_headers" yaml:"http_response_headers"` // agent building controls request headers
	TlsKey              string   `json:"tls_key" yaml:"tls_key"`
	// base64 encoded, if both empty means use self signed
	TlsCert string `json:"tls_cert" yaml:"tls_cert"`
}

type NewListenerReq struct {
	Name     string           `json:"name" yaml:"name"`
	Type     string           `json:"listener_type" yaml:"listener_type"`
	HttpInfo HttpListenerInfo `json:"http_listener_info" yaml:"http_listener_info"`
	StartNow bool             `json:"start_now" yaml:"start_now"`
}

type Arg struct {
	Type string `json:"data_type"`
	Arg  any    `json:"arg"`
}

type NewTaskReq struct {
	AgentId int    `json:"agent_id"`
	Command string `json:"command"`
	Args    []Arg  `json:"args"`
}

type ClientRegister struct {
	AgentId   int32
	AgentName string
	Username  string
	Hostname  string
	Domain    string
	ProcPath  string
	IsAdmin   int32
	Arch      int32
	Pid       int32
	Tid       int32
	PPid      int32
	WinVer    string
}

type Task struct {
	TaskId      int32 `json:"task_id"`
	CommandCode int32 `json:"command_code"`
	Args        []Arg `json:"args"`
}

type AgentTasks struct {
	Total int    `json:"total"`
	Tasks []Task `json:"tasks"`
}
