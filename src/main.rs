use actix_files as fs;
use actix_web::{get, web, App, HttpServer, Responder, HttpResponse, http::StatusCode, Result};

#[get("/")]
async fn index() ->  Result<HttpResponse> {
    Ok(HttpResponse::build(StatusCode::OK)
        .content_type("text/html; charset=utf-8")
        .body(include_str!("../static/index.html")))
}

#[get("/gen/{num}")]
async fn gen(num: web::Path<(u32)>) -> Result<fs::NamedFile> {
    Ok(fs::NamedFile::open(format!("optimal_lotto_number_sets/{}.txt", num))?)
}

#[actix_rt::main]
async fn main() -> std::io::Result<()> {
    HttpServer::new(|| 
        App::new()
            .service(index)
            .service(gen))
        .bind("127.0.0.1:8080")?
        .run()
        .await
}
