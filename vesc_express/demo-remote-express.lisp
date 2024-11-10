(def can 39)
(def esp-now-remote-mac '(132 252 230 80 168 12))

(esp-now-start)
(wifi-set-chan 1)
;84:FC:E6:50:A8:0C
(esp-now-add-peer esp-now-remote-mac) ; Add broadcast address as peer

;(print (list "starting" (get-mac-addr) (wifi-get-chan)))

(defun proc-data (src des data rssi) {
    ;(print (list "Received" src des data rssi))
    ;(print (list "Received" data))
    (def dataLen (buflen data))
    (def crc (crc16 data dataLen))
    (define data2 (array-create (+ dataLen 2)))
    (bufcpy data2 0 data 0 dataLen)
    (bufset-u16 data2 dataLen crc)
    (canmsg-send can 0 data2)
    (esp-now-send '(132 252 230 80 168 12) data)
    ;(print "Responded")
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