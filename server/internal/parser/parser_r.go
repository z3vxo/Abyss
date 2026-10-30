package parser

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"

	"github.com/z3vxo/Abyss/server/internal/shared"
)

type Reader struct {
	r   *bytes.Reader
	err error
}

func (r *Reader) Unpack4() int32 {
	if r.err != nil {
		return 0
	}
	var val int32
	r.err = binary.Read(r.r, binary.LittleEndian, &val)
	return val
}

func (r *Reader) UnpackBytes(n int32) string {
	if r.err != nil {
		return ""
	}
	if n < 0 || int64(n) > int64(r.r.Len()) {
		r.err = fmt.Errorf("invalid length %d (remaining %d)", n, r.r.Len())
		return ""
	}

	buf := make([]byte, n)
	_, r.err = io.ReadFull(r.r, buf)
	return string(buf)
}

/*
   [AgentID]        4 bytes
   [Username Len]   4 Bytes
   [Username str]   N bytes
   [Hostname Len]   4 Bytes
   [Hostname str]   N bytes
   [Domain Len]     4 Bytes
   [Domain str]     N bytes
   [ProcPath len]   4 Bytes
   [ProcPath Str]   N Bytes
   [IsAdmin]        4 Bytes
   [Arch]           4 Bytes
   [PID]            4 Bytes
   [TID]            4 Bytes
   [PPID]           4 Bytes
   [Minor]          4 Bytes
   [Major]          4 Bytes
   [Build]          4 Bytes
*/

func ParseRegister(r *bytes.Reader) (shared.ClientRegister, error) {
	rd := &Reader{r: r}

	client := shared.ClientRegister{}
	client.AgentId = rd.Unpack4()
	len := rd.Unpack4()
	client.Username = rd.UnpackBytes(len)
	len = rd.Unpack4()
	client.Hostname = rd.UnpackBytes(len)
	len = rd.Unpack4()
	client.Domain = rd.UnpackBytes(len)
	len = rd.Unpack4()
	client.ProcPath = rd.UnpackBytes(len)
	client.IsAdmin = rd.Unpack4()
	client.Arch = rd.Unpack4()
	client.Pid = rd.Unpack4()
	client.Tid = rd.Unpack4()
	client.PPid = rd.Unpack4()
	minor := rd.Unpack4()
	major := rd.Unpack4()
	build := rd.Unpack4()
	client.WinVer = MinMajorBuildToString(minor, major, build)

	if rd.err != nil {
		return shared.ClientRegister{}, rd.err
	}

	return client, nil

}

type OutputInfo struct {
	Total      int32
	OutputSize int32
	AgentId    int32
	Data       []Output
}

type Output struct {
	TaskId     int32
	OutputData string
}

var (
	OUTPUT_RAW        int32 = 4
	OUTPUT_NEED_PARSE int32 = 3
	TASK_FAILED       int32 = 5
)

type Handler func(r *Reader, out *Output) error

var TypeToHandler = map[int32]Handler{
	0xac: ParseWhoami,
}

func ParseWhoami(rd *Reader, out *Output) error {
	n := rd.Unpack4()
	if rd.err != nil {
		return rd.err
	}
	out.OutputData = rd.UnpackBytes(n)
	return rd.err
}

/*	[AgentID]     4 Bytes
	[Total Tasks] 4 Bytes
	for each task {
		[TaskId]   4 Bytes
		[OutputType] 4 Bytes  3 = Structured output(e.g ls), 4 = raw output(e.g cat); 5 = task failed, >5 succes but no output, lookup code to string in map
		if structured output:
			[PARSE_NAME] 4 Bytes; e.g PARSE_LS, PARSE_WHOAMI etc
		else if output raw:
			[OUTPUT LEN] 4 Bytes
			[Output Data] N Bytes
		per command output ...

	}
*/

func ParseOutput(r *bytes.Reader) (*OutputInfo, error) {
	rd := &Reader{r: r}
	agentID := rd.Unpack4()
	total := rd.Unpack4()
	if rd.err != nil {
		return nil, rd.err
	}
	if total < 0 || int64(total) > int64(r.Len()/8) {
		return nil, fmt.Errorf("invalid output count %d (remaining %d)", total, r.Len())
	}

	output := make([]Output, 0, total)
	for i := int32(0); i < total; i++ {
		var o Output
		o.TaskId = rd.Unpack4()
		outputType := rd.Unpack4()
		if rd.err != nil {
			return nil, rd.err
		}

		switch outputType {
		case OUTPUT_NEED_PARSE:
			parseName := rd.Unpack4()
			if rd.err != nil {
				return nil, rd.err
			}
			handler, ok := TypeToHandler[parseName]
			if !ok {
				return nil, fmt.Errorf("unknown output parser %#x", parseName)
			}
			if err := handler(rd, &o); err != nil {
				return nil, fmt.Errorf("parse output %#x: %w", parseName, err)
			}

		case OUTPUT_RAW:
			n := rd.Unpack4()
			if rd.err != nil {
				return nil, rd.err
			}
			o.OutputData = rd.UnpackBytes(n)

		case TASK_FAILED:
			errCode := rd.Unpack4()
			if rd.err != nil {
				return nil, rd.err
			}
			o.OutputData = fmt.Sprintf("task failed (error %d)", errCode)

		default:
			msg, ok := SuccessCodeToString[outputType]
			if !ok {
				return nil, fmt.Errorf("unknown output type %#x", outputType)
			}
			o.OutputData = msg
		}

		if rd.err != nil {
			return nil, rd.err
		}
		output = append(output, o)
	}

	return &OutputInfo{Total: int32(len(output)), AgentId: agentID, Data: output}, nil
}
