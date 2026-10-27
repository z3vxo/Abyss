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

func (r *Reader) Read4() int32 {
	if r.err != nil {
		return 0
	}
	var val int32
	r.err = binary.Read(r.r, binary.LittleEndian, &val)
	return val
}

func (r *Reader) ReadString(len int32) string {
	if r.err != nil {
		return ""
	}

	buf := make([]byte, len)
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
	client.AgentId = rd.Read4()
	len := rd.Read4()
	client.Username = rd.ReadString(len)
	len = rd.Read4()
	client.Hostname = rd.ReadString(len)
	len = rd.Read4()
	client.Domain = rd.ReadString(len)
	len = rd.Read4()
	client.ProcPath = rd.ReadString(len)
	client.IsAdmin = rd.Read4()
	client.Arch = rd.Read4()
	client.Pid = rd.Read4()
	client.Tid = rd.Read4()
	client.PPid = rd.Read4()
	minor := rd.Read4()
	major := rd.Read4()
	build := rd.Read4()
	client.WinVer = MinMajorBuildToString(minor, major, build)
	client.AgentName = "test-gen-soon-after-test"

	if rd.err != nil {
		return shared.ClientRegister{}, rd.err
	}

	return client, nil

}

type OutputInfo struct {
	Total   int32
	AgentId int32
	Data    []Output
}

type Output struct {
	TaskId     int32
	OutputData string
}

/*
 	[AgentId] 4 bytes
  	[Total]   4 Bytes
   	for each task [
    	[TaskId] 	  	  4 Bytes
     	[Output-Type] 	  4 Bytes // 0xaa == raw(e.g ps or cmd output), 0xcc == needs parsing(e.g ls output)
      	[per output data] N bytes
    ]

 *
*/

var (
	OUTPUT_RAW        = int32(0xaa)
	OUTPUT_NEED_PARSE = int32(0xcc)
)

type Handler func(r *Reader) error

var TypeToHandler = map[int32]Handler{
	0xbf: ParseLs,
}

func ParseLs(rd *Reader) error {
	return nil
}

func ParseOutput(r *bytes.Reader) (*OutputInfo, error) {

	rd := &Reader{r: r}
	info := &OutputInfo{
		AgentId: rd.Read4(),
		Total:   rd.Read4(),
	}
	if rd.err != nil {
		return nil, rd.err
	}
	if info.Total < 0 || int64(info.Total) > int64(r.Len()/8) {
		return nil, fmt.Errorf("invalid output count %d", info.Total)
	}

	info.Data = make([]Output, 0, info.Total)
	for range info.Total {
		var o Output
		o.TaskId = rd.Read4()
		outputType := rd.Read4()
		if rd.err != nil {
			return nil, rd.err
		}

		switch outputType {
		case OUTPUT_RAW:
			n := rd.Read4()
			if rd.err != nil {
				return nil, rd.err
			}
			if n < 0 || int64(n) > int64(r.Len()) {
				return nil, fmt.Errorf("invalid output length %d (remaining %d)", n, r.Len())
			}
			o.OutputData = rd.ReadString(n)

		case OUTPUT_NEED_PARSE:
			parserType := rd.Read4()
			if rd.err != nil {
				return nil, rd.err
			}
			parser, ok := TypeToHandler[parserType]
			if !ok {
				return nil, fmt.Errorf("unknown output parser %#x", parserType)
			}
			if err := parser(rd); err != nil {
				return nil, fmt.Errorf("parse output %#x: %w", parserType, err)
			}

		default:
			return nil, fmt.Errorf("unknown output type %#x", outputType)
		}

		if rd.err != nil {
			return nil, rd.err
		}
		info.Data = append(info.Data, o)
	}

	return info, nil
}
