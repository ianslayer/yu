(require 'cc-mode)
(require 'ido)
(require 'compile)
(ido-mode t)
(setq default-directory "d:/yu/yu")

(split-window-horizontally)
(set-foreground-color "burlywood3")
(set-background-color "#161616")
(set-cursor-color "#40FF40")

;insert tab directly
(global-set-key (kbd "TAB") 'self-insert-command)

; Turn off the bell
(defun nil-bell ())
(setq ring-bell-function 'nil-bell)

; Turn off the toolbar, menu, scroll bar
(tool-bar-mode 0)
(menu-bar-mode 0)
(toggle-scroll-bar 0)

; rebind key

(define-key global-map "\ej" 'imenu)
(define-key global-map "\eg" 'goto-line)

; find file
(define-key global-map "\ef" 'find-file)
(define-key global-map "\eF" 'find-file-other-window)

;new line indent
(define-key c++-mode-map "\C-m" 'newline-and-indent)
; reindent
(define-key c++-mode-map [C-tab] 'indent-region)


; file extensions and their appropriate modes
(setq auto-mode-alist
      (append
       '(("\\.cpp$"    . c++-mode)
         ("\\.hin$"    . c++-mode)
         ("\\.cin$"    . c++-mode)
         ("\\.inl$"    . c++-mode)
         ("\\.rdc$"    . c++-mode)
         ("\\.h$"    . c++-mode)
         ("\\.c$"   . c++-mode)
         ("\\.cc$"   . c++-mode)
         ("\\.c8$"   . c++-mode)
         ("\\.txt$" . indented-text-mode)
         ("\\.emacs$" . emacs-lisp-mode)
         ("\\.gen$" . gen-mode)
         ("\\.ms$" . fundamental-mode)
         ("\\.m$" . objc-mode)
         ("\\.mm$" . objc-mode)
         ) auto-mode-alist))


; c++ style

(defconst ianslayer-c-style
'(
  (c-offsets-alist           .(
                              (substatement-open . 0)
                              (block-open . 0)
                              )
 )
)
)

;tabs instead of spaces
(setq-default tab-width 4 indent-tabs-mode t)


(defun ianslayer-c-style-hook()
(c-add-style "ianslayer-c-setup" ianslayer-c-style t)
)

(add-hook 'c-mode-hook 'ianslayer-c-style-hook)
(add-hook 'c++-mode-hook 'ianslayer-c-style-hook)


;build environment
(load-library "view")
(require 'cc-mode)
(require 'ido)
(require 'compile)
(ido-mode t)

(setq build-script "build.bat")

(defun find-project-directory-recursive ()
  "Recursively search for a makefile."
  (interactive)
  (if (file-exists-p build-script) t
      (cd "../")
      (find-project-directory-recursive)))

(setq compilation-directory-locked nil)

(defun lock-compilation-directory ()
  "The compilation process should NOT hunt for a makefile"
  (interactive)
  (setq compilation-directory-locked t)
  (message "Compilation directory is locked."))

(defun unlock-compilation-directory ()
  "The compilation process SHOULD hunt for a makefile"
  (interactive)
  (setq compilation-directory-locked nil)
  (message "Compilation directory is roaming."))

(defun find-project-directory ()
  "Find the project directory."
  (interactive)
  (setq find-project-from-directory default-directory)
  (switch-to-buffer-other-window "*compilation*")
  (if compilation-directory-locked (cd last-compilation-directory)
  (cd find-project-from-directory)
  (find-project-directory-recursive)
  (setq last-compilation-directory default-directory)))

(defun make-without-asking ()
  "Make the current build."
  (interactive)
  (if (find-project-directory) (compile build-script))
  (other-window 1))
(define-key global-map "\em" 'make-without-asking)

(setq build-script "build.bat")

(defun find-project-directory-recursive ()
  "Recursively search for a mackfile."
  (interactive)
  (if (file-exists-p build-script) t
    (cd "../")
    (find-project-directory-recursive)
    )
  )

