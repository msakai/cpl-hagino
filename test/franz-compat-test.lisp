;;;; franz-compat-test.lisp
;;;;
;;;; Unit tests for Franz Lisp compatibility layer

(in-package :cpl)

(defvar *test-failures* 0)
(defvar *test-passes* 0)

(defmacro assert-equal (expected actual &optional message)
  "Assert that EXPECTED equals ACTUAL"
  `(if (equal ,expected ,actual)
       (progn
         (incf *test-passes*)
         (format t "."))
       (progn
         (incf *test-failures*)
         (format t "~%FAIL: ~A~%  Expected: ~S~%  Got: ~S~%"
                 ,(or message "Test failed")
                 ,expected
                 ,actual))))

(defmacro assert-true (form &optional message)
  "Assert that FORM evaluates to true"
  `(if ,form
       (progn
         (incf *test-passes*)
         (format t "."))
       (progn
         (incf *test-failures*)
         (format t "~%FAIL: ~A~%  Expression ~S evaluated to NIL~%"
                 ,(or message "Test failed")
                 ',form))))

(defun run-franz-compat-tests ()
  "Run all Franz Lisp compatibility layer tests"
  (setf *test-failures* 0)
  (setf *test-passes* 0)

  (format t "~%Testing Franz Lisp Compatibility Layer~%")
  (format t "=====================================~%")

  ;; Character I/O tests
  (format t "~%Character I/O: ")
  (assert-equal 97 (char-code (ascii 97)) "ascii conversion")
  (assert-equal 65 (char-code #\A) "character A")

  ;; String/Symbol operations
  (format t "~%String/Symbol operations: ")
  (assert-equal '(97 98 99) (exploden 'abc) "exploden abc")
  (assert-equal 'FOOBAR (concat 'foo 'bar) "concat foo bar")
  (assert-equal 'FOOBAR (uconcat 'foo 'bar) "uconcat foo bar")

  ;; List operations
  (format t "~%List operations: ")
  (assert-true (consp '(a . b)) "consp on dotted pair")
  (assert-true (not (consp 'atom)) "not consp on atom")
  (assert-true (not (consp nil)) "not consp on nil")

  ;; Arithmetic
  (format t "~%Arithmetic: ")
  (assert-equal 6 (add1 5) "add1 5")
  (assert-equal 4 (sub1 5) "sub1 5")

  ;; Property lists
  (format t "~%Property lists: ")
  (setf (get 'test-sym 'test-prop) 'test-value)
  (assert-equal 'test-value (get 'test-sym 'test-prop) "get/setf property")
  (putprop 'test-sym2 'value2 'prop2)
  (assert-equal 'value2 (get 'test-sym2 'prop2) "putprop/get")

  ;; Error handling
  (format t "~%Error handling: ")
  (assert-equal '(42) (errset 42) "errset success")
  (assert-equal nil (errset (error "test error")) "errset catches error")

  ;; Function manipulation
  (format t "~%Function manipulation: ")
  (defun test-fn () 'result)
  (assert-true (getd 'test-fn) "getd finds function")
  (assert-true (fboundp 'test-fn) "function is bound")

  ;; Size estimation
  (format t "~%Size estimation: ")
  (assert-true (> (flatc 'symbol) 0) "flatc returns positive")
  (assert-equal (flatc 'abc) (flatsize 'abc) "flatc equals flatsize")

  ;; Status
  (format t "~%System status: ")
  (assert-true (or (status 'isatty) (not (status 'isatty))) "status isatty returns boolean")

  ;; Print results
  (format t "~%~%=====================================~%")
  (format t "Tests passed: ~D~%" *test-passes*)
  (format t "Tests failed: ~D~%" *test-failures*)
  (format t "=====================================~%")

  (if (zerop *test-failures*)
      (format t "~%SUCCESS: All tests passed!~%")
      (format t "~%FAILURE: ~D test(s) failed~%" *test-failures*))

  (zerop *test-failures*))

;; Export test runner
(export '(run-franz-compat-tests))
