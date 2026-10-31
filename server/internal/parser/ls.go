package parser

import (
	"fmt"
	"strconv"
	"strings"
)

const parseLs int32 = 0xab

type lsEntry struct {
	name      string
	entryType int32
	size      uint64
}

func ParseLs(rd *Reader, out *Output) error {
	var entries []lsEntry

	for {
		nameLen := rd.Unpack4()
		if rd.err != nil {
			return rd.err
		}
		if nameLen == endSig {
			break
		}

		name := rd.UnpackBytes(nameLen)
		entryType := rd.Unpack4()
		size := rd.Unpack8()
		if rd.err != nil {
			return rd.err
		}

		entries = append(entries, lsEntry{
			name:      name,
			entryType: entryType,
			size:      size,
		})
	}

	out.OutputData = formatLs(entries)
	return nil
}

func formatLs(entries []lsEntry) string {
	type row struct {
		kind string
		size uint64
		name string
	}

	rows := make([]row, 0, len(entries))
	typeWidth := len("Type")
	sizeWidth := len("Size")
	for _, entry := range entries {
		r := row{
			kind: lsTypeToString(entry.entryType),
			size: entry.size,
			name: entry.name,
		}
		rows = append(rows, r)
		typeWidth = max(typeWidth, len(r.kind))
		sizeWidth = max(sizeWidth, len(strconv.FormatUint(r.size, 10)))
	}

	var b strings.Builder
	fmt.Fprintf(&b, "%-*s  %*s  %s\n", typeWidth, "Type", sizeWidth, "Size", "Name")
	fmt.Fprintf(&b, "%s  %s  %s\n", strings.Repeat("-", typeWidth), strings.Repeat("-", sizeWidth), "----")
	for _, r := range rows {
		fmt.Fprintf(&b, "%-*s  %*d  %s\n", typeWidth, r.kind, sizeWidth, r.size, r.name)
	}

	return b.String()
}

func lsTypeToString(entryType int32) string {
	switch entryType {
	case 1:
		return "DIR"
	case 2:
		return "LINK"
	case 3:
		return "FILE"
	default:
		return "?"
	}
}
