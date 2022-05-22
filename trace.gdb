define trace
    set pagination off

    break $arg0
    continue

    set $lr_ = $lr & ~1
    set logging file $arg1
    set logging overwrite on
    set logging redirect on
    set logging on
    while $pc != $lr_
        si
        x/i $pc
    end
    set logging off

    quit
end
