package parser

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"math"

	"github.com/z3vxo/Abyss/server/internal/shared"
)

func Pack4(w io.Writer, val int32) error {
	return binary.Write(w, binary.LittleEndian, val)
}

// PackBytes writes a length-prefixed, null-terminated string. The length
// includes the terminator, matching the agent's Parse4 len + ParseBytes pair.
func PackBytes(w io.Writer, str string) error {
	if len(str) >= math.MaxInt32 {
		return fmt.Errorf("string too large: %d bytes", len(str))
	}
	if err := Pack4(w, int32(len(str))+1); err != nil {
		return err
	}
	if _, err := w.Write([]byte(str)); err != nil {
		return err
	}
	_, err := w.Write([]byte{0})
	return err
}

func PackArg(w io.Writer, arg shared.Arg) error {
	switch arg.Type {
	case "string":
		s, ok := arg.Arg.(string)
		if !ok {
			return fmt.Errorf("arg %q: expected string, got %T", arg.Type, arg.Arg)
		}
		return PackBytes(w, s)

	case "int", "int32":
		n, err := toInt32(arg.Arg)
		if err != nil {
			return fmt.Errorf("arg %q: %w", arg.Type, err)
		}
		return Pack4(w, n)

	case "bool":
		b, ok := arg.Arg.(bool)
		if !ok {
			return fmt.Errorf("arg %q: expected bool, got %T", arg.Type, arg.Arg)
		}
		var n int32
		if b {
			n = 1
		}
		return Pack4(w, n)

	default:
		return fmt.Errorf("unknown arg type %q", arg.Type)
	}
}

func toInt32(v any) (int32, error) {
	switch n := v.(type) {
	case int:
		return int32(n), nil
	case int32:
		return n, nil
	case int64:
		return int32(n), nil
	case float64:
		return int32(n), nil
	default:
		return 0, fmt.Errorf("expected number, got %T", v)
	}
}

func PackageTasks(tasks shared.AgentTasks) ([]byte, error) {
	var buffer bytes.Buffer

	// [int32 TotalTasks]          // header, 0 => no work
	// per task:
	//   [int32 CommandCode]
	//   [int32 TaskID]
	//   ... command args ...
	if err := Pack4(&buffer, int32(len(tasks.Tasks))); err != nil {
		return nil, err
	}

	for _, task := range tasks.Tasks {
		if err := Pack4(&buffer, task.CommandCode); err != nil {
			return nil, err
		}
		if err := Pack4(&buffer, task.TaskId); err != nil {
			return nil, err
		}
		for _, arg := range task.Args {
			if err := PackArg(&buffer, arg); err != nil {
				return nil, fmt.Errorf("pack task %d: %w", task.TaskId, err)
			}
		}
	}

	return buffer.Bytes(), nil
}
