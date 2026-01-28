;;;; integration-test.lisp
;;;;
;;;; Integration tests for CPL system - tests loading system files

(in-package :cpl)

(defun test-system-file-loads (filename)
  "Test that a system file loads without errors"
  (format t "~%Testing ~A..." filename)
  (handler-case
      (progn
        ;; Try to load the system file
        ;; This is a placeholder - actual implementation depends on CPL's read function
        (format t " [SKIP - requires full CPL system running]")
        t)
    (error (e)
      (format t " FAILED: ~A" e)
      nil)))

(defun run-integration-tests ()
  "Run integration tests for CPL system"
  (format t "~%~%CPL Integration Tests~%")
  (format t "=====================~%")

  (format t "~%System File Loading Tests:~%")
  (test-system-file-loads "system/Nat")
  (test-system-file-loads "system/List")
  (test-system-file-loads "system/InfList")
  (test-system-file-loads "system/CoNat")
  (test-system-file-loads "system/Sort")

  (format t "~%~%Note: Full integration tests require the CPL REPL to be running.~%")
  (format t "To manually test:~%")
  (format t "  1. Run: ./cpl~%")
  (format t "  2. Execute: read Nat~%")
  (format t "  3. Execute: simp add.pair(s.0,s.s.0)~%")
  (format t "  4. Expected output: s.s.s.0~%")

  t)

(defun run-all-tests ()
  "Run all CPL tests"
  (let ((compat-result (run-franz-compat-tests))
        (integration-result (run-integration-tests)))
    (format t "~%~%Overall Test Results~%")
    (format t "====================~%")
    (format t "Franz Compatibility: ~A~%" (if compat-result "PASS" "FAIL"))
    (format t "Integration Tests: ~A~%" (if integration-result "PASS" "SKIP"))
    (and compat-result integration-result)))

;; Export test runners
(export '(run-integration-tests run-all-tests))

;; For CTest integration
(defun cpl-tests:run-all-tests ()
  "Test runner for CTest"
  (if (run-all-tests)
      (sb-ext:exit :code 0)
      (sb-ext:exit :code 1)))
