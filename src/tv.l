;;
;; portable lisp editor by Tatsuya Hagino
;;

(declare (macros t))

; an edit structure consists of (tail_list reverse_head_list stack)

; functions which manipulate edit structures

;(def tvhead (lambda (es) (caar es)))
(def tvhead (macro (l) `(caar ,(cadr l))))

;(def tvend (lambda (es) (atom (car es))))
(def tvend (macro (l) `(not (dtpr (car ,(cadr l))))))

(def tvfwd
 (lambda (es) (list (cdar es) (cons (caar es) (cadr es)) (caddr es))))

;(def tvbig (lambda (es) (null (cadr es))))
(def tvbig (macro (l) `(null (cadr ,(cadr l)))))

(def tvbak
 (lambda (es) (list (cons (caadr es) (car es)) (cdadr es) (caddr es))))

;(def tvtop (lambda (es) (null (caddr es))))
(def tvtop (macro (l) `(null (caddr ,(cadr l)))))

(def tvdown
 (lambda (es)
  (list (caar es) nil (cons (list (cdar es) (cadr es)) (caddr es)))))

(def tvup
 (lambda (es)
  (prog (tail stack)
	(setq stack (caddr es))
	(setq tail (cons (tvap (car es) (cadr es)) (caar stack)))
	(return (list tail (cadar stack) (cdr stack))))))

(def tvlast
 (lambda (es)
  (prog (tail head)
	(setq tail (car es))
	(setq head (cadr es))
   aa   (cond ((not (dtpr tail)) (return (list tail head (caddr es)))))
	(setq head (cons (car tail) head))
	(setq tail (cdr tail))
	(go aa))))

(def tvap
 (lambda (tail head)
  (cond ((null head) tail)
	(t (tvap (cons (car head) tail) (cdr head))))))

(def tvdel (lambda (es) (cons (cdar es) (cdr es))))

(def tvins (lambda (es s) (list (car es) (cons s (cadr es)) (caddr es))))

(def tvbot (lambda (es) (and (null (car es)) (null (caddr es)))))

(def tvnext
 (lambda (es)
  (cond ((not (tvend es))
	 (cond ((atom (tvhead es)) (tvfwd es))
	       (t (tvdown es))))
	(t (tvfwd (tvup es))))))

(def tvprev
 (lambda (es)
  (cond ((not (tvbig es))
	 (setq es (tvbak es))
	 (cond ((atom (tvhead es)) es)
	       (t (tvlast (tvdown es)))))
	(t (tvup es)))))

(def tvall
 (lambda (es)
  (cond ((tvtop es) (tvap (car es) (cadr es)))
	(t (tvall (tvup es))))))

; reading routine

(declare (special $tvcmd$))

(setq $tvcmd$ nil)

(def tvread
 (lambda (prompt)
  (prog (x)
	(cond ($tvcmd$ (setq x (car $tvcmd$))
		       (setq $tvcmd$ (cdr $tvcmd$))
		       (return x)))
   aa   (cond ((= (tyipeek) 10) (princ prompt))
	      ((< (tyipeek) 32) (tyi) (go aa)))
	(return (read)))))

(declare (special $tvfun$ $tvcur$ $tvkill$ $tvundo$ $tvchg$ $tvundop$))

(setq $tvfun$ nil)
(setq $tvcur$ "/\\")
(setq $tvkill$ nil)
(setq $tvundo$ nil)
(setq $tvundop$ t)

(def tv
 (nlambda (l)
  (prog (exp es fn cmd kill undo rtn)
	(cond ((null l) (setq fn $tvfun$))
	      (t (setq fn (car l))))
	(cond ((null fn) (error "No previous edit")))
	(setq $tvfun$ fn)
	(setq exp (getd fn))
	(cond ((null exp) (princ "New")
			  (terpri)
			  (setq es (list nil (list fn 'def) nil)))
	      ((atom exp) (error "Wrong type"))
	      (t (setq es (list (list exp) (list fn 'def) nil))))
	(setq kill $tvkill$)
	(setq $tvkill$ nil)
	(setq undo $tvundo$)
	(setq $tvundo$ nil)
	(setq $tvchg$ nil)
	(setq $tvcmd$ nil)
   aa   (setq cmd (tvread ">>"))
	(cond ((eq cmd 'ok)
	       (setq exp (tvall es))
	       (setq $tvfun$ (cadr exp))
	       (putd (cadr exp) (caddr exp))
	       (cond ((eq fn (cadr exp)) (setq rtn fn))
		     (t (setq rtn (list fn 'to (cadr exp)))))
	       (go bb))
	      ((eq cmd 'q) (setq rtn 'quit) (go bb))
	      ((eq cmd '\?) 
	       (print '(ok q t f b dr dl ur ul d k y yc s rs undo p pp ppp m e))
	       (terpri)
	       (go aa)))
	(setq es (tvcmd cmd es))
	(go aa)
   bb   (setq $tvkill$ kill)
	(setq $tvundo$ undo)
	(return rtn))))

(def tverr (lambda (es) (progn (tyo 7) (setq $tvcmd$ nil) es)))

(def tvchg
 (lambda (es)
  (cond ((and (null $tvchg$) $tvundop$)
	 (setq $tvundo$ (cons (list es $tvkill$) $tvundo$))
	 (setq $tvchg$ t)
	 es)
	(t es))))

(def tvcmd
 (lambda (cmd es)
  (cond ((numberp cmd)
	 (prog (cmdl)
	       (setq cmdl (tvread "command-list:"))
	       (cond ((atom cmdl) (return (tverr es))))
	  aa   (cond ((zerop cmd) (return es)))
	       (setq $tvcmd$ (append cmdl $tvcmd$))
	       (setq cmd (sub1 cmd))
	       (go aa)))
	((and (atom cmd) (get cmd 'tvexe))
	 (apply (get cmd 'tvexe) (list es)))
	((and (atom cmd) (get cmd 'tvcmd))
	 (setq $tvcmd$ (append (get cmd 'tvcmd) $tvcmd$))
	 es)
	(t (tvins (tvchg es) cmd)))))

(defprop f tvcmd-f tvexe)
(def tvcmd-f
 (lambda (es) (cond ((not (tvend es)) (tvfwd es)) (t (tverr es)))))

(defprop b tvcmd-b tvexe)
(def tvcmd-b
 (lambda (es) (cond ((not (tvbig es)) (tvbak es)) (t (tverr es)))))

(defprop dr tvcmd-dr tvexe)
(def tvcmd-dr
 (lambda (es)
  (prog (tes)
	(setq tes es)
   aa   (cond ((tvend tes) (return (tverr es)))
	      ((not (listp (tvhead tes)))
	       (setq tes (tvfwd tes))
	       (go aa)))
	(return (tvdown tes)))))

(defprop dl tvcmd-dl tvexe)
(def tvcmd-dl
 (lambda (es)
  (prog (tes)
	(setq tes es)
   aa   (cond ((tvbig tes) (return (tverr es)))
	      ((not (listp (tvhead (setq tes (tvbak tes))))) (go aa)))
	(return (tvlast (tvdown tes))))))

(defprop ur tvcmd-ur tvexe)
(def tvcmd-ur
 (lambda (es) (cond ((tvtop es) (tverr es)) (t (tvfwd (tvup es))))))

(defprop ul tvcmd-ul tvexe)
(def tvcmd-ul (lambda (es) (cond ((tvtop es) (tverr es)) (t (tvup es)))))

(defprop i tvcmd-i tvexe)
(def tvcmd-i (lambda (es) (tvbak (tvins (tvchg es) (tvread "insert:")))))

(defprop d tvcmd-d tvexe)
(def tvcmd-d
 (lambda (es) (cond ((tvend es) (tverr es)) (t (tvdel (tvchg es))))))

(defprop k tvcmd-k tvexe)
(def tvcmd-k
 (lambda (es)
  (cond ((tvend es) (tverr es))
	(t (tvchg es)
	   (setq $tvkill$ (cons (tvhead es) $tvkill$))
	   (tvdel es)))))

(defprop y tvcmd-y tvexe)
(def tvcmd-y
 (lambda (es)
  (prog (x)
	(cond ((null $tvkill$) (return (tverr es)))
	      (t (tvchg es)
		 (setq x (car $tvkill$))
		 (setq $tvkill$ (cdr $tvkill$))
		 (return (tvbak (tvins es x))))))))

(defprop yc tvcmd-yc tvexe)
(def tvcmd-yc
 (lambda (es)
  (cond ((null $tvkill$) (tverr es))
	(t (tvchg es)
	   (tvbak (tvins es (car $tvkill$)))))))

(defprop undo tvcmd-undo tvexe)
(def tvcmd-undo
 (lambda (es)
  (cond ((null $tvundo$) (tverr es))
	(t (setq es (caar $tvundo$))
	   (setq $tvkill$ (cadar $tvundo$))
	   (setq $tvundo$ (cdr $tvundo$))
	   (setq $tvchg$ nil)
	   es))))

(defprop p tvcmd-p tvexe)
(def tvcmd-p
 (lambda (es)
  (progn (setq $tvchg$ nil)
	 (tvpp (tvp (tvap (cons $tvcur$ (car es)) (cadr es)) 2))
	 (terpri)
	 es)))

(defprop pp tvcmd-pp tvexe)
(def tvcmd-pp
 (lambda (es)
  (progn (setq $tvchg$ nil)
	 (tvpp (tvap (cons $tvcur$ (car es)) (cadr es)))
	 (terpri)
	 es)))

(defprop ppp tvcmd-ppp tvexe)
(def tvcmd-ppp
 (lambda (es)
  (progn (tvpp (tvall (tvins es $tvcur$)))
	 (terpri)
	 es)))

(defprop s tvcmd-s tvexe)
(def tvcmd-s
 (lambda (es)
  (prog (x tes)
	(setq x (tvread "Search:"))
	(setq tes es)
   aa   (cond ((tvbot tes) (return (tverr es)))
	      ((and (not (tvend tes)) (equal (tvhead tes) x))
	       (return (tvfwd tes))))
	(setq tes (tvnext tes))
	(go aa))))

(defprop rs tvcmd-rs tvexe)
(def tvcmd-rs
 (lambda (es)
  (prog (x tes)
	(setq x (tvread "Reverse-Search:"))
	(setq tes es)
   aa   (cond ((and (tvtop tes) (tvbig tes)) (return (tverr es))))
	(setq tes (tvprev tes))
	(cond ((and (not (tvend tes)) (equal (tvhead tes) x))
	       (return tes)))
	(go aa))))

(defprop t tvcmd-t tvexe)
(def tvcmd-t
 (lambda (es)
  (prog ()
   aa   (cond ((tvtop es) (return es)))
	(setq es (tvup es))
	(go aa))))

(defprop m tvcmd-m tvexe)
(def tvcmd-m
 (lambda (es)
  (prog (nm cmdl)
	(setq nm (tvread "macro-name:"))
	(cond ((or (null nm) (not (atom nm))) (return (tverr es))))
	(setq cmdl (tvread "macro-body-list:"))
	(cond ((null cmdl) (remprop nm 'tvcmd) (return es))
	      ((atom cmdl) (return (tverr es))))
	(putprop nm cmdl 'tvcmd)
	(return es))))

(defprop e tvcmd-e tvexe)
(def tvcmd-e
 (lambda (es)
  (prog (x)
	(setq x (tvread "evaluate:"))
	(setq x (errset (eval x)))
	(cond ((not (atom x)) (print (car x)) (terpri))
	      (t (return (tverr es))))
	       (return es))))

(def tvp
 (lambda (fml dp)
  (cond ((atom fml) fml)
	((zerop dp) '|&|)
	(t (cons (tvp (car fml) (1- dp)) (tvp (cdr fml) dp))))))

(def tvpp
 (lambda (fml) (tvpp0 fml 0 (charcnt nil))))

; We should use (flatsize l max) instead of (flatc l max),
; but the latter is much faster.

(def tvpp0
 (lambda (l lmar rmar)
  (cond ((and (>& lmar 50) (>& (flatc l 101) 100))
	 (terpri)
	 (princ "; <<<<< start back on the left <<<<<")
	 (terpri)
	 (tvpp0 l 5 rmar)
	 (terpri)
	 (princ "; >>>>> continue on the right >>>>>")
	 (terpri))
	(t (tab lmar)
	   (cond ((or (not (dtpr l))
		      (and (atom (car l)) (atom (cdr l))))
		  (tvpp1 l))
		 ((<& (+ lmar (flatc l (- rmar lmar))) rmar) (tvpp1 l))
		 ((and (not (and (eq (car l) 'def) (tvpp2 l lmar rmar)))
		       (not (and (memq (car l) '(lambda nlambda lexpr macro))
				 (tvpp3 l lmar rmar)))
		       (not (and (eq (car l) 'quote) (tvpp5 l lmar rmar)))
		       (not (and (eq (car l) 'prog) (tvpp4 l lmar rmar))))
		  (prog (c)
			(princ '|(|)
			(setq lmar (1+ lmar))
			(tvpp0 (car l) lmar rmar)
			(cond ((null (cdr l)) (princ '|)|) (return nil))
			      ((atom (car l)) (princ '| |)
					      (setq lmar (nwritn nil)))
			      (t (terpri)))
		   aa   (setq l (cdr l))
			(cond ((atom l) (princ '|. |)
					(tvpp0 l (+ lmar 2) rmar)
					(princ '|)|)
					(return nil)))
			(tvpp0 (car l) lmar rmar)
			(cond ((null (cdr l)) (princ '|)|) (return nil)))
			(go aa))))))))

(def tvpp1
 (lambda (l)
  (cond ((eq l $tvcur$) (princ l))
	((atom l) (print l))
	((and (eq (car l) 'quote)
	      (not (atom (cdr l)))
	      (null (cddr l)))
	 (princ '|'|)
	 (tvpp1 (cadr l)))
	(t (princ '|(|)
	   (prog ()
	    aa   (tvpp1 (car l))
		 (setq l (cdr l))
		 (cond ((null l) (princ '|)|) (return nil))
		       ((atom l) (princ '| . |)
				 (tvpp1 l)
				 (princ '|)|)
				 (return nil)))
		 (princ '| |)
		 (go aa))))))

(def tvpp2
 (lambda (l lmar rmar)
  (cond ((and (zerop lmar)
	      (cadr l)
	      (atom (cadr l))
	      (dtpr (caddr l))
	      (null (cdddr l)))
	 (princ '|(def |)
	 (tvpp1 (cadr l))
	 (terpri)
	 (tvpp0 (caddr l) 2 rmar)
	 (princ '|)|)
	 t)
	(t nil))))

(def tvpp3
 (lambda (l lmar rmar)
  (cond ((and (not (atom (cdr l)))
	      (not (atom (cddr l)))
	      (null (cdddr l)))
	 (princ '|(|)
	 (princ (car l))
	 (princ '| |)
	 (tvpp0 (cadr l) (nwritn nil) rmar)
	 (terpri)
	 (tvpp0 (caddr l) (+ lmar 2) rmar)
	 (princ '|)|)
	 t)
	(t nil))))

(def tvpp4
 (lambda (l lmar rmar)
  (prog (c)
	(cond ((or (atom (cdr l))
		   (atom (cddr l))
		   (atom (cdddr l)))
	       (return nil)))
        (princ '|(prog |)
	(tvpp0 (cadr l) (+ lmar 6) rmar)
	(setq l (cddr l))
   aa   (cond ((null l) (princ '|)|) (return t)))
	(terpri)
	(cond ((not (atom (car l))) (go bb)))
        (tab (1+ lmar))
   cc   (tvpp1 (car l))
	(princ '| |)
	(setq l (cdr l))
	(cond ((null l) (tab (+ lmar 6)) (go aa))
	((atom (car l)) (go cc)))
   bb   (tvpp0 (car l) (+ lmar 6) rmar)
	(setq l (cdr l))
	(go aa))))

(def tvpp5
 (lambda (l lmar rmar)
  (cond ((and (not (atom (cdr l))) (null (cddr l)))
	 (princ '|'|)
	 (tvpp0 (cadr l) (1+ lmar) rmar)
	 t)
	(t nil))))

(progn (princ "TV lisp (version 1) April 1984") (terpri))
