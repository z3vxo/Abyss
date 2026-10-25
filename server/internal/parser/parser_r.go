package parser

import (
	"bytes"
	"encoding/binary"
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

	if rd.err != nil {
		return shared.ClientRegister{}, rd.err
	}

	return client, nil

}
