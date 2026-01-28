;;;; cpl.asd
;;;;
;;;; ASDF system definition for the Categorical Programming Language (CPL)
;;;; Modernized for SBCL from the original 1985-1987 Franz Lisp implementation

(asdf:defsystem #:cpl
  :description "Categorical Programming Language - A category-theoretic programming language"
  :version "4.0.0"
  :author "Tatsuya Hagino (original), Modernized for SBCL"
  :license "Historical/Academic"
  :serial t
  :components ((:file "cpl-package")
               (:file "franz-compat")
               (:file "wcathelp")
               (:file "tv")
               (:file "wmlib")
               (:file "wdia")
               (:file "wtrace")
               (:file "wcat"))
  :in-order-to ((test-op (test-op "cpl/tests"))))

(asdf:defsystem #:cpl/tests
  :description "Test suite for CPL"
  :depends-on (#:cpl)
  :serial t
  :components ((:module "tests"
                :components ((:file "franz-compat-test")
                             (:file "integration-test"))))
  :perform (test-op (o c) (symbol-call :cpl-tests :run-all-tests)))
