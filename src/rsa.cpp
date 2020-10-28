/* -------------------------------------------------------------------------- */
/* ------------- RonClient --- Oficial client for RonOTS servers ------------ */
/* -------------------------------------------------------------------------- */

#include "rsa.h"
#include <stdio.h>
#include <string>

RSA::RSA()
{
	m_keySet = false;
	mpz_init2(m_p, 1024);
	mpz_init2(m_q, 1024);
	mpz_init2(m_d, 1024);
	mpz_init2(m_u, 1024);
	mpz_init2(m_dp, 1024);
	mpz_init2(m_dq, 1024);
	mpz_init2(m_mod, 1024);
}

RSA::~RSA()
{
	mpz_clear(m_p);
	mpz_clear(m_q);
	mpz_clear(m_d);
	mpz_clear(m_u);
	mpz_clear(m_dp);
	mpz_clear(m_dq);
	mpz_clear(m_mod);
}

bool RSA::setKey(const std::string& file)
{
	LOCKCLASS lockClass(lockRsa);
	//loads p,q and d from a file
	FILE* f = fopen(file.c_str(), "r");
	if(!f){
		return false;
	}
	
	char p[512];
	char q[512];
	char d[512];
	fgets(p, 512, f);
	fgets(q, 512, f);
	fgets(d, 512, f);
	setKey(p, q, d);
	return true;
}

void RSA::setKey(const char* p, const char* q, const char* d)
{
	LOCKCLASS lockClass(lockRsa);
	
	mpz_set_str(m_p, p, 10);
	mpz_set_str(m_q, q, 10);
	mpz_set_str(m_d, d, 10);
	
	mpz_t pm1,qm1;
	mpz_init2(pm1,520);
	mpz_init2(qm1,520);

	mpz_sub_ui(pm1, m_p, 1);
	mpz_sub_ui(qm1, m_q, 1);
	mpz_invert(m_u, m_p, m_q);
	mpz_mod(m_dp, m_d, pm1);
	mpz_mod(m_dq, m_d, qm1);
	
	mpz_mul(m_mod, m_p, m_q);
	
	mpz_clear(pm1);
	mpz_clear(qm1);
}

bool RSA::encrypt(char* msg, int32_t size)
{
	LOCKCLASS lockClass(lockRsa);
	
	mpz_t plain, c;
	mpz_init2(plain, 1024);
	mpz_init2(c, 1024);

	mpz_t e;
    mpz_init(e);
	mpz_set_ui(e, 65537);

	mpz_import(plain, 128, 1, 1, 0, 0, msg);
	mpz_powm(c, plain, e, m_mod);

	size_t count = (mpz_sizeinbase(c, 2) + 7)/8;
	memset(msg, 0, 128 - count);
	mpz_export(&msg[128 - count], NULL, 1, 1, 0, 0, c);

	mpz_clear(c);
	mpz_clear(plain);
	mpz_clear(e);
	return true;
}

bool RSA::decrypt(char* msg, int32_t size)
{
	LOCKCLASS lockClass(lockRsa);
	
	mpz_t c,v1,v2,u2,tmp;
	mpz_init2(c, 1024);
	mpz_init2(v1, 1024);
	mpz_init2(v2, 1024);
	mpz_init2(u2, 1024);
	mpz_init2(tmp, 1024);
	
	mpz_import(c, 128, 1, 1, 0, 0, msg);
	
	mpz_mod(tmp, c, m_p);
	mpz_powm(v1, tmp, m_dp, m_p);
	mpz_mod(tmp, c, m_q);
	mpz_powm(v2, tmp, m_dq, m_q);
	mpz_sub(u2, v2, v1);
	mpz_mul(tmp, u2, m_u);
	mpz_mod(u2, tmp, m_q);
	if(mpz_cmp_si(u2, 0) < 0){
		mpz_add(tmp, u2, m_q);
		mpz_set(u2, tmp);
	}
	mpz_mul(tmp, u2, m_p);
	mpz_set_ui(c, 0);
	mpz_add(c, v1, tmp);
	
	size_t count = (mpz_sizeinbase(c, 2) + 7)/8;
	memset(msg, 0, 128 - count);
	mpz_export(&msg[128 - count], NULL, 1, 1, 0, 0, c);
	
	mpz_clear(c);
	mpz_clear(v1);
	mpz_clear(v2);
	mpz_clear(u2);
	mpz_clear(tmp);
	
	return true;
}

int32_t RSA::getKeySize()
{
	LOCKCLASS lockClass(lockRsa);
	
	size_t count = (mpz_sizeinbase(m_mod, 2) + 7)/8;
	int32_t a = count/128;
	return a*128;
}

void RSA::getPublicKey(char* buffer)
{
	LOCKCLASS lockClass(lockRsa);
	
	size_t count = (mpz_sizeinbase(m_mod, 2) + 7)/8;
	memset(buffer, 0, 128 - count);
	mpz_export(&buffer[128 - count], NULL, 1, 1, 0, 0, m_mod);
}
