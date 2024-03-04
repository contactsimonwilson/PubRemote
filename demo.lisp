(esp-now-start)
(wifi-set-chan 1)
;84:FC:E6:50:A8:0C
(define remote-mac '(132 252 230 80 168 12))
(esp-now-add-peer remote-mac) ; Add broadcast address as peer

(print (list "Starting" (get-mac-addr) (wifi-get-chan)))

(defun proc-data (src des data rssi) {
    ;(print (list "Received" src des data rssi))
    (print (list "Received" data))
    (esp-now-send remote-mac data)
    }
)

(defun event-handler ()
    (loopwhile t
        (recv
            ((event-esp-now-rx (? src) (? des) (? data) (? rssi)) (proc-data src des data rssi))
            (_ nil)
)))

(event-register-handler (spawn event-handler))
(event-enable 'event-esp-now-rx)

