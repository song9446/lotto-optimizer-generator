//use actix_files as fs;
use actix_web::{get, web, App, HttpServer, Responder, HttpResponse, http::StatusCode, Result};

#[get("/")]
async fn index() ->  Result<HttpResponse> {
    Ok(HttpResponse::build(StatusCode::OK)
        .content_type("text/html; charset=utf-8")
        .body(include_str!("../static/index.html")))
}

#[get("/expectation?")]
async fn expectation() -> impl Responder {
    format!("Hello")
}

#[actix_rt::main]
async fn main() -> std::io::Result<()> {
    HttpServer::new(|| 
        App::new()
            .service(index)
            .service(expectation))
        .bind("127.0.0.1:8080")?
        .run()
        .await
}
