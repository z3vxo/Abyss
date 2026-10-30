package parser

import "fmt"

func MinMajorBuildToString(minor, major, build int32) string {
	switch {
	case major == 10 && minor == 0:
		switch {
		case build >= 26100:
			return "Windows 11, version 24H2 or later"
		case build >= 22631:
			return "Windows 11, version 23H2"
		case build >= 22621:
			return "Windows 11, version 22H2"
		case build >= 22000:
			return "Windows 11"
		case build >= 19045:
			return "Windows 10, version 22H2"
		case build >= 19044:
			return "Windows 10, version 21H2"
		case build >= 19043:
			return "Windows 10, version 22H1"
		case build >= 19042:
			return "Windows 10, version 20H2"
		case build >= 19041:
			return "Windows 10, version 2004"
		case build >= 10240:
			return "Windows 10"
		default:
			return fmt.Sprintf("Windows NT %d.%d (Build %d", major, minor, build)
		}
	case major == 6 && minor == 3:
		return "Windows 8.1"
	case major == 6 && minor == 2:
		return "Windows 8"
	case major == 6 && minor == 1:
		return "Windows 7"
	case major == 6 && minor == 0:
		return "Windows Vista"
	case major == 5 && minor == 2:
		return "Windows XP Professional x64 / Windows Server 2003"
	case major == 5 && minor == 1:
		return "Windows XP"
	case major == 5 && minor == 0:
		return "Windows 2000"
	default:
		return fmt.Sprintf("Windows NT %d.%d (Build %d)", major, minor, build)
	}
}

var (
	SuccessCodeToString = map[int32]string{
		6: "Successfully moved files!",
	}
)
