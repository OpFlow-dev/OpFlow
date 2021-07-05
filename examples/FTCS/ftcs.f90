program main
    implicit none

    real*8 u0(100001), u1(100001), dx(100001), idx(100001)

    integer i, j

    dx = 100 / 1000000.
    idx = 1./ dx

    do i = 1, 10000
        u1(1) = 0
        u1(100001) = 1
        do j = 2, 100000
            u1(j) = 1e-16 * 1 * (u0(j - 1) - 2 * u0(j) + u0(j + 1)) * (idx(j) * idx(j))
        end do
        u0 = u1
    end do
end
