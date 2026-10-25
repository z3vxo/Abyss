package cli

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
