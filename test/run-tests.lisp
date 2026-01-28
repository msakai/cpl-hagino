;;;; run-tests.lisp
;;;;
;;;; Standalone test runner for CPL

(require :asdf)

;; Add source directory to ASDF registry
(push #p"/Users/sakai/src/cpl-hagino/src/" asdf:*central-registry*)

;; Load the CPL system
(format t "Loading CPL system...~%")
(asdf:load-system :cpl)

;; Load test files
(format t "Loading test suite...~%")
(load "/Users/sakai/src/cpl-hagino/test/franz-compat-test.lisp")
(load "/Users/sakai/src/cpl-hagino/test/integration-test.lisp")

;; Run tests
(in-package :cpl)
(let ((result (run-all-tests)))
  (if result
      (progn
        (format t "~%~%All tests passed!~%")
        (sb-ext:exit :code 0))
      (progn
        (format t "~%~%Some tests failed!~%")
        (sb-ext:exit :code 1))))
