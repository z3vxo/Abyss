package parser

import (
	"fmt"
	"strings"
)

/*
	[NameLen]
	[NameStr]
	[DomainLen]
	[DomainStr]
	[SidLen]
	[SidStr]
	for each priv
		[priv name len]
		[priv name str]
		[priv status]
	[end_sig] 0xDEADBEEF
*/

const parseWhoami int32 = 0xac

type whoamiPrivilege struct {
	name   string
	status int32
}

func ParseWhoami(rd *Reader, out *Output) error {
	name := rd.UnpackString()
	domain := rd.UnpackString()
	sid := rd.UnpackString()
	if rd.err != nil {
		return rd.err
	}

	var privileges []whoamiPrivilege
	for {
		val := rd.Unpack4()
		if rd.err != nil {
			return rd.err
		}
		if val == endSig {
			break
		}

		privName := rd.UnpackBytes(val)
		status := rd.Unpack4()
		if rd.err != nil {
			return rd.err
		}

		privileges = append(privileges, whoamiPrivilege{name: privName, status: status})
	}

	out.OutputData = formatWhoami(name, domain, sid, privileges)
	return nil
}

func formatWhoami(name, domain, sid string, privileges []whoamiPrivilege) string {
	user := name
	if domain != "" {
		user = domain + `\` + name
	}

	var b strings.Builder

	userWidth := max(len("User Name"), len(user))
	sidWidth := max(len("SID"), len(sid))

	fmt.Fprintf(&b, "USER INFORMATION\n----------------\n\n")
	fmt.Fprintf(&b, "%-*s %s\n", userWidth, "User Name", "SID")
	fmt.Fprintf(&b, "%s %s\n", strings.Repeat("=", userWidth), strings.Repeat("=", sidWidth))
	fmt.Fprintf(&b, "%-*s %s\n", userWidth, user, sid)

	fmt.Fprintf(&b, "\nPRIVILEGES INFORMATION\n----------------------\n\n")

	type privilegeRow struct {
		name        string
		description string
		state       string
	}

	rows := make([]privilegeRow, 0, len(privileges))
	nameWidth := len("Privilege Name")
	descWidth := len("Description")
	for _, p := range privileges {
		row := privilegeRow{
			name:        p.name,
			description: privilegeDescriptions[p.name],
			state:       privilegeStateToString(p.status),
		}
		rows = append(rows, row)
		nameWidth = max(nameWidth, len(row.name))
		descWidth = max(descWidth, len(row.description))
	}

	fmt.Fprintf(&b, "%-*s %-*s %s\n", nameWidth, "Privilege Name", descWidth, "Description", "State")
	fmt.Fprintf(&b, "%s %s %s\n", strings.Repeat("=", nameWidth), strings.Repeat("=", descWidth), "========")
	for _, row := range rows {
		fmt.Fprintf(&b, "%-*s %-*s %s\n", nameWidth, row.name, descWidth, row.description, row.state)
	}

	return b.String()
}

var privilegeDescriptions = map[string]string{
	"SeAssignPrimaryTokenPrivilege":             "Replace a process level token",
	"SeAuditPrivilege":                          "Generate security audits",
	"SeBackupPrivilege":                         "Back up files and directories",
	"SeChangeNotifyPrivilege":                   "Bypass traverse checking",
	"SeCreateGlobalPrivilege":                   "Create global objects",
	"SeCreatePagefilePrivilege":                 "Create a pagefile",
	"SeCreatePermanentPrivilege":                "Create permanent shared objects",
	"SeCreateSymbolicLinkPrivilege":             "Create symbolic links",
	"SeCreateTokenPrivilege":                    "Create a token object",
	"SeDebugPrivilege":                          "Debug programs",
	"SeDelegateSessionUserImpersonatePrivilege": "Obtain an impersonation token for another user in the same session",
	"SeEnableDelegationPrivilege":               "Enable computer and user accounts to be trusted for delegation",
	"SeImpersonatePrivilege":                    "Impersonate a client after authentication",
	"SeIncreaseBasePriorityPrivilege":           "Increase scheduling priority",
	"SeIncreaseQuotaPrivilege":                  "Adjust memory quotas for a process",
	"SeIncreaseWorkingSetPrivilege":             "Increase a process working set",
	"SeLoadDriverPrivilege":                     "Load and unload device drivers",
	"SeLockMemoryPrivilege":                     "Lock pages in memory",
	"SeMachineAccountPrivilege":                 "Add workstations to domain",
	"SeManageVolumePrivilege":                   "Perform volume maintenance tasks",
	"SeProfileSingleProcessPrivilege":           "Profile single process",
	"SeRelabelPrivilege":                        "Modify an object label",
	"SeRemoteShutdownPrivilege":                 "Force shutdown from a remote system",
	"SeRestorePrivilege":                        "Restore files and directories",
	"SeSecurityPrivilege":                       "Manage auditing and security log",
	"SeShutdownPrivilege":                       "Shut down the system",
	"SeSyncAgentPrivilege":                      "Synchronize directory service data",
	"SeSystemEnvironmentPrivilege":              "Modify firmware environment values",
	"SeSystemProfilePrivilege":                  "Profile system performance",
	"SeSystemtimePrivilege":                     "Change the system time",
	"SeTakeOwnershipPrivilege":                  "Take ownership of files or other objects",
	"SeTcbPrivilege":                            "Act as part of the operating system",
	"SeTimeZonePrivilege":                       "Change the time zone",
	"SeTrustedCredManAccessPrivilege":           "Access Credential Manager as a trusted caller",
	"SeUndockPrivilege":                         "Remove computer from docking station",
}

func privilegeStateToString(status int32) string {
	switch status {
	case 0:
		return "Removed"
	case 1:
		return "Enabled"
	case 2:
		return "Disabled (by default)"
	default:
		return "Disabled"
	}
}
