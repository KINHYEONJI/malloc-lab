# C언어의 `malloc`과 `free`를 직접 구현하는 프로젝트입니다.

<br>

### 직접 정리한 블로그 link
[동적할당 vs 정적할당 ](https://velog.io/@hyeon_zip/%EB%8F%99%EC%A0%81%ED%95%A0%EB%8B%B9-vs-%EC%A0%95%EC%A0%81%ED%95%A0%EB%8B%B9)

[묵시적 vs 명시적](https://velog.io/@hyeon_zip/C%EC%96%B8%EC%96%B4-%EB%AC%B5%EC%8B%9C%EC%A0%81Implicit-vs-%EB%AA%85%EC%8B%9C%EC%A0%81explicit)

[Implicit-free-list 알아보기](https://velog.io/@hyeon_zip/C%EC%96%B8%EC%96%B4-malloc-%EA%B5%AC%ED%98%84%ED%95%98%EA%B8%B0)

[Explicit-free-list 알아보기]
---

### 세팅방법
```
# ubuntu 환경
sudo apt update
sudo apt install build-essential
sudo apt install gdb
sudo apt-get install gcc-multilib g++-multilib

git clone https://github.com/KINHYEONJI/malloc-lab.git
```
---
## 파일 설명

implicit(first-fit, next-fit), explicit, segregated-fit 총 `4가지 방식`으로 구현하였습니다.

- mm.c 파일에는 implicit(first-fit)방식이 구현되어 있습니다.

- mms 디렉토리에는 implicit(next-fit), explicit, segregated-fit 3가지 방식으로 구현한 파일이 있습니다.

---
## 실행방법
```
#mm.c 실행방법
make
./mdriver

# mms에 있는 다른 파일의 내용을 복사하여 mm.c에 붙여넣으면 다른 방식으로 테스트 하는 방법
make clear
make
./mdriver
```
