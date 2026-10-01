# Permissions

Permissions are evaluated per authorization request. `PermissionPolicyService`, `PermissionService`, `IApprovalPolicy`, and `SQLitePermissionGrantStore` separate policy evaluation, user approval, session grants, and persisted allow grants. The policy registry identifies workspace access, tool and agent execution, voice capture/playback, cloud-provider access, filesystem writes, subprocess execution, memory commit, and context injection; actual enforcement remains descriptor- and resource-specific.

Session decisions can allow or deny a matching request for that session. Persistent grants are allow-only and only accepted for bounded filesystem-path, host, provider, or argument-digest scopes. The Desktop exposes runtime permission posture and persistent-grant revoke/clear actions. A denied or revoked permission prevents the relevant execution; it does not complete the agent run. Plugin permissions are declared in manifests and checked again at invocation.

The default policy state governs external-service requests; other requests still use descriptor requirements, risk policy, explicit grants, and resource checks. The CLI currently exposes no equivalent interactive approval workflow. Use Desktop for reviewing and changing user-visible permission state. Permission grants do not replace resource authorization or sandbox checks.
