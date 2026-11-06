add_test([=[EventLog.AppendAndRetrieve]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=EventLog.AppendAndRetrieve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[EventLog.AppendAndRetrieve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_events.cpp:8]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[EventLog.Range]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=EventLog.Range]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[EventLog.Range]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_events.cpp:19]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[EventLog.Clear]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=EventLog.Clear]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[EventLog.Clear]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_events.cpp:26]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[EventLog.TypeIndexTracksAppendAndClear]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=EventLog.TypeIndexTracksAppendAndClear]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[EventLog.TypeIndexTracksAppendAndClear]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_events.cpp:33]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Storage.RoundTrip]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=Storage.RoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Storage.RoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_storage.cpp:9]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Timeline.Navigation]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=Timeline.Navigation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Timeline.Navigation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_timeline.cpp:8]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Timeline.CheckpointAndFind]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=Timeline.CheckpointAndFind]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Timeline.CheckpointAndFind]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_timeline.cpp:21]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Timeline.SeekValidatesAndReturnsEvent]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=Timeline.SeekValidatesAndReturnsEvent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Timeline.SeekValidatesAndReturnsEvent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_timeline.cpp:32]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Timeline.SeekOnEmptyOrDetachedLogReturnsNoEvent]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=Timeline.SeekOnEmptyOrDetachedLogReturnsNoEvent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Timeline.SeekOnEmptyOrDetachedLogReturnsNoEvent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_timeline.cpp:44]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[StateDiff.BuildAndDiff]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=StateDiff.BuildAndDiff]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[StateDiff.BuildAndDiff]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_state_diff.cpp:8]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PtraceTracer.TracesEcho]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=PtraceTracer.TracesEcho]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PtraceTracer.TracesEcho]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_ptrace.cpp:8]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CheckpointRestore.RestoresCapturedBytesAndPermissions]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=CheckpointRestore.RestoresCapturedBytesAndPermissions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CheckpointRestore.RestoresCapturedBytesAndPermissions]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_checkpoint_restore.cpp:6]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CheckpointRestore.RejectsInvalidCapturedRegion]=]  /home/cristopher/chronos/build_san/tests/chronos_tests [==[--gtest_filter=CheckpointRestore.RejectsInvalidCapturedRegion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CheckpointRestore.RejectsInvalidCapturedRegion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/cristopher/chronos/tests/test_checkpoint_restore.cpp:26]==]
    WORKING_DIRECTORY [==[/home/cristopher/chronos/build_san/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
set(chronos_tests_TESTS [==[EventLog.AppendAndRetrieve]==] [==[EventLog.Range]==] [==[EventLog.Clear]==] [==[EventLog.TypeIndexTracksAppendAndClear]==] [==[Storage.RoundTrip]==] [==[Timeline.Navigation]==] [==[Timeline.CheckpointAndFind]==] [==[Timeline.SeekValidatesAndReturnsEvent]==] [==[Timeline.SeekOnEmptyOrDetachedLogReturnsNoEvent]==] [==[StateDiff.BuildAndDiff]==] [==[PtraceTracer.TracesEcho]==] [==[CheckpointRestore.RestoresCapturedBytesAndPermissions]==] [==[CheckpointRestore.RejectsInvalidCapturedRegion]==])
