# Integration test ownership

The nine stages in [bitcoin/bitcoin#36289](https://github.com/bitcoin/bitcoin/issues/36289)
work as test ownership boundaries. The fixture from
[gui-qml#939](https://github.com/bitcoin-core/gui-qml/pull/939) starts the production
application and a real regtest node for every test function or data row. Tests can
drive QML controls, inspect the actual models and signals, and compare their state
with Core RPC results without expanding the Python automation bridge.

| Stage | Directory | Current integration coverage | Remaining stage acceptance work |
| --- | --- | --- | --- |
| 1. Foundation | `stage1_foundation` | Startup, shutdown, application lifetime, pending backend work | Packaging, platform startup and fatal-exit checks |
| 2. Node setup and shell | `stage2_node_setup` | Navigation, settings, language and units, block clock, synchronization and network pause | Pre-init onboarding, persisted settings and restart journeys; tray and native menus |
| 3. Node tools | `stage3_node_tools` | Core network notifications and modal messages, live debug log, console commands/autocomplete/layout, blocks-only navigation | Multi-node peer/ban/proxy/traffic journeys and broader mempool inspection |
| 4. Wallet management | `stage4_wallet_management` | Name validation, create/encrypt/unlock/relock, close/reload and model destruction, backup/restore and invalid-backup UI | Legacy migration, cross-version compatibility and native file dialogs |
| 5. Wallet activity | `stage5_wallet_activity` | Receive requests, private notes, labels propagated to activity/address/request models, address types, sign/verify | Complete transaction history, export and QR journeys |
| 6. Wallet sending | `stage6_wallet_sending` | Custom fees under two relay/wallet policies, review/broadcast/mempool fees, coin selection and locks | Full multi-recipient and multi-node send/receive journeys |
| 7. PSBT and replacement | `stage7_psbt` | Import/review/export/sign/broadcast PSBT, foreign PSBT restrictions, fee-bump confirmation/replacement | Complete offline and cross-wallet PSBT journeys |
| 8. Watch-only and signers | `stage8_signers` | Watch-only creation, invalid keys, unsigned transaction/export restrictions | External signer subprocess interaction, device errors and unavailable devices |
| 9. Payment request import | `stage9_payment_import` | Clipboard conflicts, draft preservation, malformed URI rejection, path/file-URL import | OS command-line/open-file entry points and argument-injection checks |

Stages 1–3 run in both wallet modes; stages 4–9 run with wallets enabled. The
table describes test coverage, not implementation completeness. A stage label
does not demonstrate that the application can be compiled with later-stage code
removed. Staging PRs still need their own build and dependency checks.

Put cross-stage scenarios in the latest stage whose behavior they exercise:
watch-only PSBT export belongs to stage 8 and imported-payment review belongs to
stage 9. Earlier stages should not depend on helpers that require a loaded wallet.
Keep shared process/UI helpers in `fixture/test_util.h` and wallet setup helpers
in `fixture/test_wallet.h`. RPC is useful for arranging chain/wallet state and
checking results; exercise the UI or public model operation under test directly.
The nine CTest entries retain function/data-row isolation and may run in parallel.

## Functional-test migration

These Python scripts are superseded by integration coverage and removed from the
functional workflow:

| Former script (`test/functional/`) | Integration module |
| --- | --- |
| `qml_test_blockclock.py` | `blockclock` |
| `qml_test_blocksonly_settings.py` | `blocksonly` |
| `qml_test_console.py` | `console` |
| `qml_test_debug_log.py` | `debug-log` |
| `qml_test_node_runtime_dialogs.py` | `node` (real Core UI signals) |
| `qml_test_create_wallet_name.py` | `wallet` |
| `qml_test_address_label_sync.py` | `receive` |
| `qml_test_custom_fee.py` | `send` |
| `qml_test_watchonly_wallet.py` | `watchonly`, `wallet` |

Component tests remain the owner of mock-based layout edge cases, such as debug
log scrolling, and wallet-creation flow variants. Integration tests cover their
connection to live models. The remaining Python tests still cover longer journeys
and OS/process boundaries: restarts, onboarding, other nodes, legacy binaries,
external signers, native dialogs, command-line imports, shutdown and tray behavior.
Some overlap is intentional while those journeys remain intact; adding a new
model assertion should normally extend the corresponding integration module
instead of adding bridge properties or another Python script.

## Sanitizers

Reuse the existing build directory, generator and compiler cache. For example:

```sh
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_APP_TESTS=ON -DWITH_CCACHE=ON -DENABLE_IPC=OFF \
  -DSANITIZERS=address,undefined,integer,implicit-conversion,local-bounds,nullability,float-divide-by-zero
cmake --build build --target bitcoinqml_integration_tests -j12
cat bitcoin/test/sanitizer_suppressions/ubsan test/sanitizer_suppressions/ubsan > build/ubsan.supp
```

Run with `ASAN_OPTIONS=halt_on_error=1:detect_stack_use_after_return=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1:report_error_type=1:suppressions=PATH`,
where `PATH` is the absolute path to `build/ubsan.supp`. Core's file documents
intentional integer wrapping in cryptography and dependencies; the GUI additions
cover Qt's deliberate unsigned hash arithmetic. Pass these environment values
with `cmake -E env`, followed by `ctest --test-dir build -L integration -j3
--output-on-failure`. The parent passes the environment to discovery and test
children, so sanitizer failures fail their rows and retain diagnostics.

ThreadSanitizer requires a separate build configuration with `-DSANITIZERS=thread`;
it cannot run with AddressSanitizer. When switching sanitizer configurations with
Makefiles, run `cmake --build build --target clean` before rebuilding: secp256k1
appends sanitizer flags to its C compile rule, and stale objects can retain the
previous instrumentation. This preserves the build configuration and compiler cache. System Qt libraries may be uninstrumented,
which limits concurrency coverage and can produce reports around Qt synchronization.
Inspect each report before introducing a narrowly justified dependency suppression.
Never suppress an entire application module merely to make the run pass.

Availability is compiler/platform dependent: Apple Clang on macOS arm64 does not
support standalone LeakSanitizer, MemorySanitizer or HWAddressSanitizer. MemorySanitizer
also requires instrumented dependencies on supported platforms. Consult
[Clang's sanitizer documentation](https://clang.llvm.org/docs/UsersManual.html#controlling-code-generation)
before adding flags. These runs only exercise the selected integration scenarios;
they do not establish the absence of memory or concurrency defects.

Save reports from `build/test-reports/` and failed profiles before changing
configurations. Restore the regular build with `-DSANITIZERS=` and rebuild the
targets you use. Live debug-log tests require OS file-change notifications; a
restricted environment that blocks them cannot validate live updates.
